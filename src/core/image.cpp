#include "image.h"
#include <fstream>
#include <cstring>

namespace oci {

Image::Image()
    : mWidth(0), mHeight(0), mLogicalWidth(0), mLogicalHeight(0),
      mHasColorKey(false), mColorKey(0) {
}

void Image::create(unsigned width, unsigned height, const Color& color) {
    mLogicalWidth = static_cast<int>(width);
    mLogicalHeight = static_cast<int>(height);
    mWidth = static_cast<int>(width);
    mHeight = static_cast<int>(height);
    if(mWidth < 1) mWidth = 1;
    if(mHeight < 1) mHeight = 1;

    uint16_t c = color.toRGB565();
    mPixels.assign(static_cast<size_t>(mWidth) * mHeight, c);
}

void Image::createMaskFromColor(Color color_key) {
    mHasColorKey = true;
    mColorKey = color_key.toRGB565();
}

static inline uint16_t make_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

bool Image::loadFromMemory(const char* data, std::size_t datasize) {
    if(!data || datasize < 54)
        return false;

    const uint8_t* udata = reinterpret_cast<const uint8_t*>(data);
    if(udata[0] != 'B' || udata[1] != 'M')
        return false;

    uint32_t off_bits = udata[10] | (udata[11] << 8) | (udata[12] << 16) | (udata[13] << 24);
    int32_t bi_width = static_cast<int32_t>(udata[18] | (udata[19] << 8) | (udata[20] << 16) | (udata[21] << 24));
    int32_t bi_height = static_cast<int32_t>(udata[22] | (udata[23] << 8) | (udata[24] << 16) | (udata[25] << 24));
    uint16_t bi_bpp = udata[28] | (udata[29] << 8);

    if(bi_width <= 0 || bi_height == 0)
        return false;

    bool bottom_up = true;
    if(bi_height < 0) {
        bottom_up = false;
        bi_height = -bi_height;
    }

    mLogicalWidth = bi_width;
    mLogicalHeight = bi_height;
    mWidth = bi_width;
    mHeight = bi_height;

    if(off_bits >= datasize)
        return false;

    mPixels.resize(static_cast<size_t>(mWidth) * mHeight);

    if(bi_bpp == 24) {
        size_t row_stride = (static_cast<size_t>(bi_width) * 3 + 3) & ~static_cast<size_t>(3);
        if(off_bits + row_stride * static_cast<size_t>(bi_height) > datasize)
            return false;
        for(int dy = 0; dy < mHeight; ++dy) {
            size_t row_idx = bottom_up ? static_cast<size_t>(bi_height - 1 - dy) : static_cast<size_t>(dy);
            const uint8_t* row = udata + off_bits + row_idx * row_stride;
            for(int dx = 0; dx < mWidth; ++dx) {
                const uint8_t* p = row + dx * 3;
                mPixels[static_cast<size_t>(dy) * mWidth + dx] = make_rgb565(p[2], p[1], p[0]);
            }
        }
        return true;
    } else if(bi_bpp == 8) {
        const uint8_t* palette = udata + 14 + 40;
        size_t row_stride = (static_cast<size_t>(bi_width) + 3) & ~static_cast<size_t>(3);
        if(off_bits + row_stride * static_cast<size_t>(bi_height) > datasize)
            return false;
        for(int dy = 0; dy < mHeight; ++dy) {
            size_t row_idx = bottom_up ? static_cast<size_t>(bi_height - 1 - dy) : static_cast<size_t>(dy);
            const uint8_t* row = udata + off_bits + row_idx * row_stride;
            for(int dx = 0; dx < mWidth; ++dx) {
                uint8_t idx = row[dx];
                const uint8_t* pe = palette + static_cast<size_t>(idx) * 4;
                mPixels[static_cast<size_t>(dy) * mWidth + dx] = make_rgb565(pe[2], pe[1], pe[0]);
            }
        }
        return true;
    }

    return false;
}

bool Image::loadFromFile(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
    if(!file.is_open())
        return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(size));
    if(!file.read(buffer.data(), size))
        return false;

    return loadFromMemory(buffer.data(), buffer.size());
}

} // namespace oci
