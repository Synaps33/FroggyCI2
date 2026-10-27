#include "player.h"
#include "controller.h"
#include <resources/loader.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace oci {
namespace audio {

static constexpr int TARGET_RATE = 44100;
static constexpr int MAX_VOICES = 16;

struct SoundBuffer {
    std::vector<int16_t> samples;
};

struct Voice {
    const SoundBuffer* buffer = nullptr;
    // 64-bit Q16 position: 32 bits overflow after 65536 samples (~1.5 s @44.1 kHz).
    uint64_t pos_q16 = 0;
    uint32_t step_q16 = 0;
    bool loop = false;
    bool active = false;
    uint32_t generation = 0;
};

static Voice s_voices[MAX_VOICES];
static uint32_t s_voice_generation[MAX_VOICES] = { 0 };
static std::unordered_map<std::string, SoundBuffer> s_buffer_cache;

static uint16_t rd16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

static uint32_t rd32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static bool parse_wav(const uint8_t* raw, size_t size, SoundBuffer& out_buf) {
    if(!raw || size < 44 || std::memcmp(raw, "RIFF", 4) != 0 || std::memcmp(raw + 8, "WAVE", 4) != 0)
        return false;

    int format = 1;
    int channels = 1;
    uint32_t rate = 22050;
    int bits = 16;
    const uint8_t* data = nullptr;
    uint32_t data_size = 0;

    size_t pos = 12;
    while(pos + 8 <= size) {
        const uint8_t* cid = raw + pos;
        uint32_t csize = rd32(raw + pos + 4);
        const uint8_t* body = raw + pos + 8;
        if(pos + 8 + csize > size)
            break;

        if(std::memcmp(cid, "fmt ", 4) == 0 && csize >= 16) {
            format = rd16(body);
            channels = rd16(body + 2);
            rate = rd32(body + 4);
            bits = rd16(body + 14);
        } else if(std::memcmp(cid, "data", 4) == 0) {
            data = body;
            data_size = csize;
        }
        pos += 8 + csize + (csize & 1);
    }

    if(format != 1 || !data || data_size == 0 || channels <= 0 || (bits != 8 && bits != 16))
        return false;

    size_t num_source_samples = data_size / (channels * (bits / 8));
    if(num_source_samples == 0)
        return false;

    // Convert source to 16-bit mono
    std::vector<int16_t> mono_source(num_source_samples);
    for(size_t i = 0; i < num_source_samples; ++i) {
        int32_t acc = 0;
        for(int c = 0; c < channels; ++c) {
            if(bits == 8) {
                uint8_t b = data[(i * channels + c)];
                acc += (static_cast<int32_t>(b) - 128) << 8;
            } else {
                const uint8_t* p = data + (i * channels + c) * 2;
                acc += static_cast<int16_t>(rd16(p));
            }
        }
        mono_source[i] = static_cast<int16_t>(acc / channels);
    }

    // Resample to TARGET_RATE (44100 Hz) using linear interpolation
    uint32_t step_q16 = static_cast<uint32_t>(((static_cast<uint64_t>(rate)) << 16) / TARGET_RATE);
    size_t num_target_samples = (static_cast<uint64_t>(num_source_samples) * TARGET_RATE) / rate;
    out_buf.samples.resize(num_target_samples);

    uint64_t cur_pos_q16 = 0;
    for(size_t i = 0; i < num_target_samples; ++i) {
        size_t idx = static_cast<size_t>(cur_pos_q16 >> 16);
        if(idx + 1 < num_source_samples) {
            uint32_t frac = cur_pos_q16 & 0xFFFF;
            int32_t s0 = mono_source[idx];
            int32_t s1 = mono_source[idx + 1];
            out_buf.samples[i] = static_cast<int16_t>(s0 + (((s1 - s0) * static_cast<int32_t>(frac)) >> 16));
        } else if(idx < num_source_samples) {
            out_buf.samples[i] = mono_source[idx];
        } else {
            out_buf.samples[i] = 0;
        }
        cur_pos_q16 += step_q16;
    }

    return true;
}

static const SoundBuffer* get_sound_buffer(std::string_view filename) {
    std::string key(filename);
    auto it = s_buffer_cache.find(key);
    if(it != s_buffer_cache.end())
        return &it->second;

    std::string path = "sfx/" + key;
    std::vector<char> raw;
    if(!resources::ResourceLoader::Instance().TryGetData(path, raw) || raw.empty())
        return nullptr;

    SoundBuffer buf;
    if(!parse_wav(reinterpret_cast<const uint8_t*>(raw.data()), raw.size(), buf))
        return nullptr;

    auto res = s_buffer_cache.emplace(std::move(key), std::move(buf));
    return &res.first->second;
}

class VoiceController : public Controller {
public:
    VoiceController(int voice_idx, uint32_t gen)
        : mVoiceIdx(voice_idx), mGeneration(gen) {}

    ~VoiceController() override {
        Stop();
    }

    void Stop() override {
        if(mVoiceIdx >= 0 && mVoiceIdx < MAX_VOICES &&
           s_voice_generation[mVoiceIdx] == mGeneration) {
            s_voices[mVoiceIdx].active = false;
            s_voices[mVoiceIdx].buffer = nullptr;
        }
    }

    bool IsPlaying() const override {
        if(mVoiceIdx >= 0 && mVoiceIdx < MAX_VOICES) {
            return s_voices[mVoiceIdx].active &&
                   (s_voice_generation[mVoiceIdx] == mGeneration);
        }
        return false;
    }

private:
    int mVoiceIdx;
    uint32_t mGeneration;
};

std::unique_ptr<Controller> Play(std::string_view filename, bool autoplay, bool loop) {
    const SoundBuffer* buf = get_sound_buffer(filename);
    if(!buf || buf->samples.empty())
        return std::make_unique<VoiceController>(-1, 0);

    int voice_idx = -1;
    for(int i = 0; i < MAX_VOICES; ++i) {
        if(!s_voices[i].active) {
            voice_idx = i;
            break;
        }
    }
    if(voice_idx < 0) {
        // Steal the oldest voice
        static int s_steal_robin = 0;
        voice_idx = s_steal_robin;
        s_steal_robin = (s_steal_robin + 1) % MAX_VOICES;
    }

    uint32_t gen = ++s_voice_generation[voice_idx];
    Voice& v = s_voices[voice_idx];
    v.buffer = buf;
    v.pos_q16 = 0;
    v.step_q16 = 1 << 16;
    v.loop = loop;
    v.active = autoplay;
    v.generation = gen;

    return std::make_unique<VoiceController>(voice_idx, gen);
}

void StopAll() {
    for(int i = 0; i < MAX_VOICES; ++i) {
        s_voices[i].active = false;
        s_voices[i].buffer = nullptr;
    }
}

void StopLoops() {
    for(int i = 0; i < MAX_VOICES; ++i) {
        if(s_voices[i].active && s_voices[i].loop) {
            s_voices[i].active = false;
            s_voices[i].buffer = nullptr;
        }
    }
}

void AudioMix(int16_t* out_stereo, size_t frames) {
    if(!out_stereo || frames == 0)
        return;

    std::memset(out_stereo, 0, frames * 2 * sizeof(int16_t));

    for(int v_idx = 0; v_idx < MAX_VOICES; ++v_idx) {
        Voice& v = s_voices[v_idx];
        if(!v.active || !v.buffer || v.buffer->samples.empty())
            continue;

        const int16_t* samples = v.buffer->samples.data();
        const size_t total_samples = v.buffer->samples.size();
        size_t cur_pos = static_cast<size_t>(v.pos_q16 >> 16);

        for(size_t f = 0; f < frames; ++f) {
            if(cur_pos >= total_samples) {
                if(v.loop) {
                    cur_pos = 0;
                    v.pos_q16 = 0;
                } else {
                    v.active = false;
                    break;
                }
            }

            int32_t s = samples[cur_pos];

            int32_t l = out_stereo[f * 2 + 0] + s;
            int32_t r = out_stereo[f * 2 + 1] + s;

            if(l < -32768) l = -32768;
            else if(l > 32767) l = 32767;
            if(r < -32768) r = -32768;
            else if(r > 32767) r = 32767;

            out_stereo[f * 2 + 0] = static_cast<int16_t>(l);
            out_stereo[f * 2 + 1] = static_cast<int16_t>(r);

            v.pos_q16 += v.step_q16;
            cur_pos = static_cast<size_t>(v.pos_q16 >> 16);
        }
    }
}

} // namespace audio
} // namespace oci
