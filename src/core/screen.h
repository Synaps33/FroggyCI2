#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace oci {
namespace core {

class Screen {
public:
    // Logical resolution used by the game (original SDL build).
    static constexpr int WIDTH = 640;
    static constexpr int HEIGHT = 480;
    // Physical resolution of the SF2000 / GB300 panel.
    static constexpr int OUT_WIDTH = 320;
    static constexpr int OUT_HEIGHT = 240;

    static Screen& Instance();

    uint16_t* GetBuffer() { return mBuffer; }
    const uint16_t* GetBuffer() const { return mBuffer; }

    void Clear(uint16_t color = 0);

    void Blit(int dst_x, int dst_y, const uint16_t* src, int src_w, int src_h,
              bool has_colorkey, uint16_t colorkey);

    void FillRect(int dst_x, int dst_y, int w, int h, uint16_t color);

    // 2x2 box filter, RGB565 -> RGB565 at half resolution.
    void DownscaleToOutput(uint16_t* out) const;

private:
    Screen();
    uint16_t mBuffer[WIDTH * HEIGHT];
};

} // namespace core
} // namespace oci
