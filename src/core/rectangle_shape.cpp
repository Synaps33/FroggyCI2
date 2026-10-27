#include "rectangle_shape.h"
#include "color.h"
#include "screen.h"

namespace oci {

void RectangleShape::DoDraw(void* /*target*/) const {
    int sx = static_cast<int>(getPosition().x);
    int sy = static_cast<int>(getPosition().y);
    int sw = static_cast<int>(mSize.x);
    int sh = static_cast<int>(mSize.y);
    core::Screen::Instance().FillRect(sx, sy, sw, sh, mFillColor.toRGB565());
}

} // namespace oci
