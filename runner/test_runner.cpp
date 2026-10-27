#include "libretro.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>

#define W 320
#define H 240

static uint16_t g_fb[W * H];
static uint16_t g_pad = 0;
static int g_frame = 0;
static int g_video_calls = 0;
static int64_t g_audio_frames = 0;
static int64_t g_audio_nonzero = 0;

static void video_cb(const void* data, unsigned width, unsigned height, size_t pitch) {
    if(!data) return;
    const uint8_t* src = static_cast<const uint8_t*>(data);
    for(unsigned y = 0; y < height && y < H; ++y) {
        unsigned n = width < W ? width : W;
        std::memcpy(&g_fb[y * W], src + y * pitch, n * sizeof(uint16_t));
    }
    g_video_calls++;
}

static void audio_sample_cb(int16_t /*l*/, int16_t /*r*/) {
}

static size_t audio_batch_cb(const int16_t* data, size_t frames) {
    g_audio_frames += static_cast<int64_t>(frames);
    if(data) {
        for(size_t i = 0; i < frames * 2; ++i) {
            if(data[i] != 0)
                g_audio_nonzero++;
        }
    }
    return frames;
}

static void input_poll_cb(void) {
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)port; (void)index;
    if(device != RETRO_DEVICE_JOYPAD)
        return 0;
    return (g_pad & (1u << id)) ? 1 : 0;
}

static bool environ_cb(unsigned cmd, void* data) {
    if(cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
        enum retro_pixel_format fmt = *reinterpret_cast<enum retro_pixel_format*>(data);
        return fmt == RETRO_PIXEL_FORMAT_RGB565;
    }
    return true;
}

static void save_ppm(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if(!f) return;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    uint8_t rgb[W * 3];
    for(int y = 0; y < H; ++y) {
        for(int x = 0; x < W; ++x) {
            uint16_t p = g_fb[y * W + x];
            rgb[x * 3 + 0] = ((p >> 11) & 0x1F) << 3;
            rgb[x * 3 + 1] = ((p >> 5) & 0x3F) << 2;
            rgb[x * 3 + 2] = (p & 0x1F) << 3;
        }
        fwrite(rgb, 1, sizeof(rgb), f);
    }
    fclose(f);
}

static int count_nonblack_pixels() {
    int count = 0;
    for(int i = 0; i < W * H; ++i) {
        if(g_fb[i] != 0)
            count++;
    }
    return count;
}

static uint32_t calc_checksum() {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(g_fb);
    for(size_t i = 0; i < sizeof(g_fb); ++i) {
        crc ^= p[i];
        for(int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

int main(int argc, char** argv) {
    const char* game_dir = (argc > 1) ? argv[1] : ".";
    int total_frames = (argc > 2) ? std::atoi(argv[2]) : 400;
    const char* sshot_prefix = (argc > 3) ? argv[3] : "screenshots/frame";
    const char* mode = (argc > 4) ? argv[4] : "tour";
    const bool continue_mode = std::strcmp(mode, "continue") == 0;
    const bool reset_mode = std::strcmp(mode, "reset") == 0;

    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    printf("=== Open Chicken Invaders Runner ===\n");
    printf("Game directory: %s\n", game_dir);
    printf("Total frames: %d (mode: %s)\n", total_frames, mode);

    retro_set_environment(environ_cb);
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_sample_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);

    retro_init();

    struct retro_game_info info;
    std::string rom_path = std::string(game_dir) + "/ChickenInvaders2.dat";
    info.path = rom_path.c_str();
    info.data = nullptr;
    info.size = 0;
    info.meta = nullptr;

    if(!retro_load_game(&info)) {
        fprintf(stderr, "Failed to load game!\n");
        return 1;
    }
    printf("Game loaded successfully!\n");

    for(g_frame = 0; g_frame < total_frames; ++g_frame) {
        // Scripted inputs:
        // Frame 0-25: Menu idle
        // Frame 26-35: Press Fire/Start ("save the world")
        // Frame 36-120: Intro cutscene
        // Frame 121-130: Press Escape (Select) to skip intro
        // Frame 131-200: Round 1 intro
        // Frame 201-400: Gameplay (move Left/Right, Fire)

        g_pad = 0;
        if(continue_mode) {
            // Continue is the first menu entry when a save exists.
            if(g_frame >= 60 && g_frame <= 75) {
                g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_START) | (1u << RETRO_DEVICE_ID_JOYPAD_A);
            } else if(g_frame > 200) {
                if((g_frame / 90) % 2 == 0)
                    g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_LEFT);
                else
                    g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT);
                g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_A);
            }
        } else if(reset_mode) {
            if(g_frame >= 60 && g_frame <= 75)
                g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_START) | (1u << RETRO_DEVICE_ID_JOYPAD_A);
        } else {
            // Deterministic tour of the game.
            if(g_frame >= 60 && g_frame <= 75) {
                g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_START) | (1u << RETRO_DEVICE_ID_JOYPAD_A);
            } else if(g_frame >= 200 && g_frame <= 215) {
                // Skip intro with X (Return in game terms).
                g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_X);
            } else if(g_frame > 260) {
                if((g_frame / 90) % 2 == 0)
                    g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_LEFT);
                else
                    g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT);
                g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_A);
                if(g_frame % 150 == 0)
                    g_pad |= (1u << RETRO_DEVICE_ID_JOYPAD_X) | (1u << RETRO_DEVICE_ID_JOYPAD_UP);
            }
        }

        if(reset_mode && (g_frame == 200 || g_frame == 400)) {
            printf("[Frame %d] retro_reset()\n", g_frame);
            retro_reset();
        }

        retro_run();

        struct Shot { int frame; const char* suffix; };
        const Shot tour_shots[] = {
            { 55, "01_menu" }, { 150, "02_intro" }, { 300, "03_round_start" },
            { 500, "04_gameplay" }, { 900, "05_wave" }, { -1, "06_final" },
            { 0, nullptr }
        };
        const Shot continue_shots[] = {
            { 55, "01_menu" }, { 130, "02_continued" }, { 500, "03_gameplay" },
            { 900, "04_wave" }, { -1, "05_final" },
            { 0, nullptr }
        };
        const Shot* shots = continue_mode ? continue_shots : tour_shots;
        for(const Shot* shot = shots; shot->suffix; ++shot) {
            const int frame = (shot->frame < 0) ? total_frames - 1 : shot->frame;
            if(g_frame == frame) {
                char name[256];
                snprintf(name, sizeof(name), "%s_%s.ppm", sshot_prefix, shot->suffix);
                save_ppm(name);
                printf("[Frame %d] Saved %s (non-black: %d, crc: 0x%08X)\n",
                       g_frame, name, count_nonblack_pixels(), calc_checksum());
            }
        }
    }

    printf("\n=== Execution Summary ===\n");
    printf("Total frames rendered: %d\n", g_frame);
    printf("Video callback calls: %d\n", g_video_calls);
    printf("Audio frames generated: %lld (nonzero samples: %lld)\n",
           (long long)g_audio_frames, (long long)g_audio_nonzero);
    printf("Final frame nonblack pixels: %d / %d\n", count_nonblack_pixels(), W * H);
    printf("Final frame CRC32: 0x%08X\n", calc_checksum());

    retro_unload_game();
    retro_deinit();
    return 0;
}
