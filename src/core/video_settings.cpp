#include "video_settings.h"
#include <resources/path_helper.h>
#include <core/boot_log.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace oci {

namespace {

VideoResolution s_resolution = VideoResolution::Res320x240;
bool s_initialized = false;

std::string ConfigPath() {
    return ResolvePath("video.cfg");
}

void SaveConfig() {
    std::string path = ConfigPath();
    FILE* f = std::fopen(path.c_str(), "w");
    if(f) {
        std::fprintf(f, "resolution=%s\n",
                     s_resolution == VideoResolution::Res640x480 ? "640x480" : "320x240");
        std::fclose(f);
        BootLog("video config saved to %s: %s", path.c_str(),
                s_resolution == VideoResolution::Res640x480 ? "640x480" : "320x240");
    }
}

void LoadConfig() {
    std::string path = ConfigPath();
    FILE* f = std::fopen(path.c_str(), "r");
    if(f) {
        char buf[64];
        if(std::fgets(buf, sizeof(buf), f)) {
            if(strstr(buf, "640x480")) {
                s_resolution = VideoResolution::Res640x480;
            } else if(strstr(buf, "320x240")) {
                s_resolution = VideoResolution::Res320x240;
            }
        }
        std::fclose(f);
        BootLog("video config loaded from %s: %s", path.c_str(),
                s_resolution == VideoResolution::Res640x480 ? "640x480" : "320x240");
    }
}

} // namespace

namespace VideoSettings {

void Init() {
    if(!s_initialized) {
        s_initialized = true;
        LoadConfig();
    }
}

VideoResolution GetResolution() {
    Init();
    return s_resolution;
}

void SetResolution(VideoResolution res) {
    s_resolution = res;
    s_initialized = true;
    SaveConfig();
}

void ToggleResolution() {
    Init();
    if(s_resolution == VideoResolution::Res320x240) {
        SetResolution(VideoResolution::Res640x480);
    } else {
        SetResolution(VideoResolution::Res320x240);
    }
}

const char* GetResolutionName() {
    return GetResolution() == VideoResolution::Res640x480 ? "640x480" : "320x240";
}

} // namespace VideoSettings
} // namespace oci
