#pragma once

#include "drawable.h"
#include "rect.h"
#include <cassert>

namespace oci {
    class Texture;
} // namespace oci

namespace oci::core {

class Sprite : public Drawable {
public:
    Sprite() = default;
    Sprite(const Texture& tex) {
        setTexture(tex, true);
    }

    const Texture* getTexture() const noexcept { return mTexture; }
    void setTexture(const Texture& texture, bool resetRect = false) {
        (void)resetRect;
        mTexture = &texture;
    }

    FloatRect getGlobalBounds() const noexcept;

    void DoDraw(void* target = nullptr) const override;

private:
    const Texture* mTexture = nullptr;
};

} // namespace oci::core
