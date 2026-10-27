#include "ogg_decoder.h"

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_HEADER_ONLY
#include "../../third_party/stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace oci {
namespace resources {

namespace {

void put16(std::vector<char>& out, uint16_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
}

void put32(std::vector<char>& out, uint32_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
    out.push_back(static_cast<char>((v >> 16) & 0xFF));
    out.push_back(static_cast<char>((v >> 24) & 0xFF));
}

} // namespace

bool DecodeOggToWav(const std::vector<char>& file, std::vector<char>& wav) {
    if(file.empty() || file.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return false;

    int channels = 0;
    int sample_rate = 0;
    short* samples = nullptr;
    int frames = stb_vorbis_decode_memory(
        reinterpret_cast<const unsigned char*>(file.data()),
        static_cast<int>(file.size()),
        &channels, &sample_rate, &samples);
    if(frames <= 0 || !samples || channels <= 0 || channels > 2 ||
       sample_rate <= 0 || sample_rate > 192000) {
        std::free(samples);
        return false;
    }

    const uint32_t data_size = static_cast<uint32_t>(
        static_cast<uint64_t>(frames) * static_cast<uint64_t>(channels) * 2u);
    const uint32_t riff_size = 36u + data_size;
    wav.clear();
    wav.reserve(44u + data_size);
    wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
    put32(wav, riff_size);
    wav.insert(wav.end(), {'W', 'A', 'V', 'E'});
    wav.insert(wav.end(), {'f', 'm', 't', ' '});
    put32(wav, 16);
    put16(wav, 1);
    put16(wav, static_cast<uint16_t>(channels));
    put32(wav, static_cast<uint32_t>(sample_rate));
    put32(wav, static_cast<uint32_t>(sample_rate) * static_cast<uint32_t>(channels) * 2u);
    put16(wav, static_cast<uint16_t>(channels * 2));
    put16(wav, 16);
    wav.insert(wav.end(), {'d', 'a', 't', 'a'});
    put32(wav, data_size);
    const char* raw = reinterpret_cast<const char*>(samples);
    wav.insert(wav.end(), raw, raw + data_size);
    std::free(samples);
    return true;
}

} // namespace resources
} // namespace oci
