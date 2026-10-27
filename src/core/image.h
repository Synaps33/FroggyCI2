#pragma once

#include "color.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace oci {

class Image {
public:
    Image();
    ~Image() = default;

    void create(unsigned width, unsigned height, const Color& color);
    bool loadFromMemory(const char* data, std::size_t datasize);
    bool loadFromFile(const std::string& filename);
    void createMaskFromColor(Color color_key);

    int getWidth() const noexcept { return mWidth; }
    int getHeight() const noexcept { return mHeight; }
    int getLogicalWidth() const noexcept { return mLogicalWidth; }
    int getLogicalHeight() const noexcept { return mLogicalHeight; }

    const uint16_t* getPixels() const noexcept { return mPixels.data(); }
    bool hasColorKey() const noexcept { return mHasColorKey; }
    uint16_t getColorKey() const noexcept { return mColorKey; }

private:
    int mWidth;
    int mHeight;
    int mLogicalWidth;
    int mLogicalHeight;
    std::vector<uint16_t> mPixels;
    bool mHasColorKey;
    uint16_t mColorKey;
};

} // namespace oci
