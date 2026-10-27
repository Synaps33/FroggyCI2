#include "texture.h"
#include "image.h"

namespace oci {

Texture::Texture()
    : mWidth(0), mHeight(0), mLogicalWidth(0), mLogicalHeight(0),
      mHasColorKey(false), mColorKey(0) {
}

bool Texture::loadFromImage(const Image& img) {
    mWidth = img.getWidth();
    mHeight = img.getHeight();
    mLogicalWidth = img.getLogicalWidth();
    mLogicalHeight = img.getLogicalHeight();
    mHasColorKey = img.hasColorKey();
    mColorKey = img.getColorKey();

    if(mWidth > 0 && mHeight > 0 && img.getPixels()) {
        mPixels = std::make_shared<std::vector<uint16_t>>(
            img.getPixels(), img.getPixels() + (mWidth * mHeight));
        return true;
    }
    return false;
}

Vector2u Texture::getSize() const {
    return Vector2u(mLogicalWidth, mLogicalHeight);
}

} // namespace oci
