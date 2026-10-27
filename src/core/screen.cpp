#include "screen.h"

namespace oci {
namespace core {

namespace {

// Average two RGB565 pixels without unpacking channels: works per bit-field.
static inline uint16_t avg565(uint16_t a, uint16_t b) {
    return static_cast<uint16_t>(((a ^ b) & 0xF7DEu) >> 1) + (a & b);
}

} // namespace

Screen& Screen::Instance() {
    static Screen instance;
    return instance;
}

Screen::Screen() {
    Clear(0);
}

void Screen::Clear(uint16_t color) {
    if(color == 0) {
        std::memset(mBuffer, 0, sizeof(mBuffer));
    } else {
        for(int i = 0; i < WIDTH * HEIGHT; ++i)
            mBuffer[i] = color;
    }
}

void Screen::Blit(int dst_x, int dst_y, const uint16_t* src, int src_w, int src_h,
                  bool has_colorkey, uint16_t colorkey) {
    if(!src || src_w <= 0 || src_h <= 0)
        return;

    int w = src_w;
    int h = src_h;
    int sx = 0;
    int sy = 0;

    if(dst_x < 0) {
        sx = -dst_x;
        w += dst_x;
        dst_x = 0;
    }
    if(dst_y < 0) {
        sy = -dst_y;
        h += dst_y;
        dst_y = 0;
    }
    if(dst_x + w > WIDTH)
        w = WIDTH - dst_x;
    if(dst_y + h > HEIGHT)
        h = HEIGHT - dst_y;

    if(w <= 0 || h <= 0)
        return;

    if(!has_colorkey && sx == 0 && w == src_w) {
        // Fast path: whole rows, no transparency.
        for(int y = 0; y < h; ++y)
            std::memcpy(&mBuffer[(dst_y + y) * WIDTH + dst_x],
                        &src[(sy + y) * src_w], w * sizeof(uint16_t));
        return;
    }

    for(int y = 0; y < h; ++y) {
        uint16_t* drow = &mBuffer[(dst_y + y) * WIDTH + dst_x];
        const uint16_t* srow = &src[(sy + y) * src_w + sx];
        if(has_colorkey) {
            for(int x = 0; x < w; ++x) {
                uint16_t p = srow[x];
                if(p != colorkey)
                    drow[x] = p;
            }
        } else {
            std::memcpy(drow, srow, w * sizeof(uint16_t));
        }
    }
}

void Screen::FillRect(int dst_x, int dst_y, int w, int h, uint16_t color) {
    if(w <= 0 || h <= 0)
        return;

    if(dst_x < 0) {
        w += dst_x;
        dst_x = 0;
    }
    if(dst_y < 0) {
        h += dst_y;
        dst_y = 0;
    }
    if(dst_x + w > WIDTH)
        w = WIDTH - dst_x;
    if(dst_y + h > HEIGHT)
        h = HEIGHT - dst_y;

    if(w <= 0 || h <= 0)
        return;

    for(int y = 0; y < h; ++y) {
        uint16_t* drow = &mBuffer[(dst_y + y) * WIDTH + dst_x];
        for(int x = 0; x < w; ++x)
            drow[x] = color;
    }
}

void Screen::DownscaleToOutput(uint16_t* out) const {
    for(int y = 0; y < OUT_HEIGHT; ++y) {
        const uint16_t* row0 = &mBuffer[(y * 2) * WIDTH];
        const uint16_t* row1 = row0 + WIDTH;
        uint16_t* dst = &out[y * OUT_WIDTH];
        for(int x = 0; x < OUT_WIDTH; ++x) {
            uint16_t top = avg565(row0[x * 2], row0[x * 2 + 1]);
            uint16_t bottom = avg565(row1[x * 2], row1[x * 2 + 1]);
            dst[x] = avg565(top, bottom);
        }
    }
}

} // namespace core
} // namespace oci
