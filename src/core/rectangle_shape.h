#pragma once

#include "color.h"
#include "drawable.h"
#include "vector2.h"

namespace oci {

class RectangleShape : public Drawable {
public:
    RectangleShape(Vector2f size = Vector2f(0, 0))
        : mSize(size), mFillColor(Color::Black)
    {}

    void setFillColor(Color color) { mFillColor = color; }

    void DoDraw(void* target = nullptr) const override;

private:
    Vector2f mSize;
    Color mFillColor;
};

} // namespace oci
