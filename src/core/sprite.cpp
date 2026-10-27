#include "sprite.h"
#include "rect.h"
#include "screen.h"
#include "texture.h"
#include "vector2.h"

namespace oci::core {

FloatRect Sprite::getGlobalBounds() const noexcept {
    const Vector2u tex_size = mTexture ? mTexture->getSize() : Vector2u();
    return { getPosition() - getOrigin(), tex_size };
}

void Sprite::DoDraw(void* /*target*/) const {
    if(mTexture && mTexture->getPixels()) {
        int dst_x = static_cast<int>(getPosition().x - getOrigin().x);
        int dst_y = static_cast<int>(getPosition().y - getOrigin().y);
        Screen::Instance().Blit(
            dst_x, dst_y,
            mTexture->getPixels(),
            mTexture->getWidth(),
            mTexture->getHeight(),
            mTexture->hasColorKey(),
            mTexture->getColorKey()
        );
    }
}

} // namespace oci::core
