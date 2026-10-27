#pragma once

#include "vector2.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace oci {

class Image;

class Texture {
public:
    Texture();
    ~Texture() = default;

    bool loadFromImage(const Image& img);

    Vector2u getSize() const;

    int getWidth() const noexcept { return mWidth; }
    int getHeight() const noexcept { return mHeight; }
    int getLogicalWidth() const noexcept { return mLogicalWidth; }
    int getLogicalHeight() const noexcept { return mLogicalHeight; }

    const uint16_t* getPixels() const noexcept { return mPixels ? mPixels->data() : nullptr; }
    bool hasColorKey() const noexcept { return mHasColorKey; }
    uint16_t getColorKey() const noexcept { return mColorKey; }

private:
    int mWidth;
    int mHeight;
    int mLogicalWidth;
    int mLogicalHeight;
    std::shared_ptr<std::vector<uint16_t>> mPixels;
    bool mHasColorKey;
    uint16_t mColorKey;
};

} // namespace oci
