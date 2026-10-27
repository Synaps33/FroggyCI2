#include "libretro.h"
#include <audio/player.h>
#include <background/background.h>
#include <context/context.h>
#include <context/manager.h>
#include <core/keyboard.h>
#include <core/mouse.h>
#include <core/screen.h>
#include <core/video_mode.h>
#include <core/window.h>
#include <resources/loader.h>
#include <core/video_settings.h>
#include <core/window_style.h>
#include <menu/mainmenu.h>
#include <levels/manager.h>
#include <core/boot_log.h>
#include <resources/path_helper.h>

#include <cstring>
#include <string>

using namespace oci;

static retro_environment_t s_environ;
static retro_video_refresh_t s_video;
static retro_audio_sample_t s_audio;
static retro_audio_sample_batch_t s_audio_batch;
static retro_input_poll_t s_input_poll;
static retro_input_state_t s_input_state;

static bool s_loaded = false;

static void extract_dir(const char* path, char* out, size_t out_size) {
    if(!path) {
        snprintf(out, out_size, ".");
        return;
    }
    const char* slash = strrchr(path, '/');
    if(!slash) slash = strrchr(path, '\\');
    if(!slash) {
        snprintf(out, out_size, ".");
    } else {
        size_t len = slash - path;
        if(len >= out_size) len = out_size - 1;
        memcpy(out, path, len);
        out[len] = '\0';
    }
}

static void check_core_options() {
    if(!s_environ) return;
    struct retro_variable var;
    var.key = "oci_resolution";
    var.value = nullptr;
    if(s_environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if(strcmp(var.value, "640x480") == 0) {
            VideoSettings::SetResolution(VideoResolution::Res640x480);
        } else if(strcmp(var.value, "320x240") == 0) {
            VideoSettings::SetResolution(VideoResolution::Res320x240);
        }
    }
}

void retro_set_environment(retro_environment_t cb) {
    s_environ = cb;
    if(s_environ) {
        enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
        s_environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

        static const struct retro_variable vars[] = {
            { "oci_resolution", "Output Resolution; 320x240|640x480" },
            { nullptr, nullptr }
        };
        s_environ(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
    }
    oci::BootLog("retro_set_environment");
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    s_video = cb;
    Window::SetVideoCallback(cb);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    s_audio = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    s_audio_batch = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    s_input_poll = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    s_input_state = cb;
}

void retro_init(void) {
    oci::BootLog("retro_init");
}

void retro_deinit(void) {
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info) {
    memset(info, 0, sizeof(*info));
    info->library_name = "Open Chicken Invaders";
    info->library_version = "1.2.1";
    info->valid_extensions = "gba|bin|zip|dat";
    info->need_fullpath = true;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    memset(info, 0, sizeof(*info));
    info->geometry.base_width = core::Screen::WIDTH;
    info->geometry.base_height = core::Screen::HEIGHT;
    info->geometry.max_width = core::Screen::WIDTH;
    info->geometry.max_height = core::Screen::HEIGHT;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps = 60.0;
    info->timing.sample_rate = 44100.0;
}

void retro_set_controller_port_device(unsigned /*port*/, unsigned /*device*/) {
}

void retro_reset(void) {
    if(s_loaded) {
        MainMenu::SwitchToMenu();
    }
}

void retro_run(void) {
    if(!s_loaded)
        return;
    static int s_run_count = 0;
    if(s_run_count < 5) {
        oci::BootLog("retro_run #%d", s_run_count + 1);
        ++s_run_count;
    }

    if(s_input_poll)
        s_input_poll();

    if(s_input_state) {
        auto btn = [](unsigned id) -> bool {
            return s_input_state(0, RETRO_DEVICE_JOYPAD, 0, id) != 0;
        };

        bool left = btn(RETRO_DEVICE_ID_JOYPAD_LEFT);
        bool right = btn(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        bool up = btn(RETRO_DEVICE_ID_JOYPAD_UP);
        bool down = btn(RETRO_DEVICE_ID_JOYPAD_DOWN);
        bool a = btn(RETRO_DEVICE_ID_JOYPAD_A);
        bool b = btn(RETRO_DEVICE_ID_JOYPAD_B);
        bool x = btn(RETRO_DEVICE_ID_JOYPAD_X);
        bool y = btn(RETRO_DEVICE_ID_JOYPAD_Y);
        bool start = btn(RETRO_DEVICE_ID_JOYPAD_START);
        bool select = btn(RETRO_DEVICE_ID_JOYPAD_SELECT);

        Keyboard::SetKeyPressed(Keyboard::Left, left);
        Keyboard::SetKeyPressed(Keyboard::Right, right);
        Keyboard::SetKeyPressed(Keyboard::Up, up);
        Keyboard::SetKeyPressed(Keyboard::Down, down);
        Keyboard::SetKeyPressed(Keyboard::Return, x || y || start);
        Keyboard::SetKeyPressed(Keyboard::Fire, a || b);
        Keyboard::SetKeyPressed(Keyboard::Escape, select);

        Mouse::SetButtonPressed(Mouse::Left, a || b || start);
    }

    // The original game logic runs at 25 fps (Window::setFramerateLimit(25)).
    // The panel/frontend calls retro_run at 60 Hz, so tick the simulation with
    // a fixed accumulator and repeat the last video frame in between.
    static int s_accumulator = 0;
    s_accumulator += 25;
    bool tick = false;
    while(s_accumulator >= 60) {
        s_accumulator -= 60;
        tick = true;
    }

    if(tick) {
        static int s_tick_count = 0;
        if(s_tick_count < 5) {
            oci::BootLog("tick #%d", s_tick_count + 1);
            ++s_tick_count;
        }
        Window::Instance().clear();
        Background::Instance().Draw();
        context::Context* context = context::Manager::Instance().GetActiveContext();
        if(context) {
            context->Run();
            context->ProcessCollisions();
            context->Animate();
            context->Draw();
        }
        MainMenu::ProcessPending();
        Window::Instance().display();
        levels::Manager::Instance().TickAutosave();
    } else {
        Window::Instance().present();
    }

    // 44100 Hz / 60 video frames = 735 stereo frames per retro_run call.
    if(s_audio_batch) {
        int16_t audio_buf[735 * 2];
        audio::AudioMix(audio_buf, 735);
        s_audio_batch(audio_buf, 735);
    }
}

bool retro_load_game(const struct retro_game_info *info) {
    oci::BootLog("retro_load_game path=%s", (info && info->path) ? info->path : "(null)");
    if(info && info->path) {
        char base_dir[1024];
        extract_dir(info->path, base_dir, sizeof(base_dir));
        SetBaseDirectory(base_dir);
        oci::resources::ResourceLoader::Instance().SetPreferredDat(info->path);
        oci::BootLog("base_dir=%s", base_dir);
    }

    check_core_options();
    oci::BootLog("window create");
    VideoMode vm(640, 480);
    Window::Instance().create(vm, "Chicken Invaders", Style::Close);
    Window::Instance().setFramerateLimit(60);
    oci::BootLog("main menu init");
    MainMenu::InitMenu();
    oci::BootLog("main menu ready");

    s_loaded = true;
    return true;
}

bool retro_load_game_special(unsigned /*type*/, const struct retro_game_info* /*info*/, size_t /*num_info*/) {
    return false;
}

void retro_unload_game(void) {
    levels::Manager::Instance().FlushProgress();
    audio::StopAll();
    s_loaded = false;
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

void *retro_get_memory_data(unsigned /*id*/) {
    return nullptr;
}

size_t retro_get_memory_size(unsigned /*id*/) {
    return 0;
}

size_t retro_serialize_size(void) {
    return 0;
}

bool retro_serialize(void* /*data*/, size_t /*size*/) {
    return false;
}

bool retro_unserialize(const void* /*data*/, size_t /*size*/) {
    return false;
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned /*index*/, bool /*enabled*/, const char* /*code*/) {
}
