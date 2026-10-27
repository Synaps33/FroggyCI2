#pragma once

namespace oci {

enum class VideoResolution {
    Res320x240, // Output 320x240 (640x480 box-filtered downscaled)
    Res640x480  // Output native 640x480
};

namespace VideoSettings {
    void Init();
    VideoResolution GetResolution();
    void SetResolution(VideoResolution res);
    void ToggleResolution();
    const char* GetResolutionName();
}

} // namespace oci
