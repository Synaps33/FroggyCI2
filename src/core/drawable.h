#pragma once

#include "vector2.h"

namespace oci {

class Drawable {
public:
    Drawable() {}
    Drawable(const Vector2f& position) : mPosition(position) {}

    virtual ~Drawable() = default;

    const Vector2f& getPosition() const { return mPosition; }
    void setPosition(float x, float y) { mPosition.x = x; mPosition.y = y; }
    void setPosition(const Vector2f& position) { mPosition = position; }
    void move(float dx, float dy) { mPosition.x += dx; mPosition.y += dy; }
    void move(const Vector2f& delta) { move(delta.x, delta.y); }

    const Vector2f& getOrigin() const { return mOrigin; }
    void setOrigin(float x, float y) { mOrigin.x = x; mOrigin.y = y; }
    void setOrigin(const Vector2f& point) { mOrigin = point; }

    virtual void DoDraw(void* target = nullptr) const = 0;

private:
    Vector2f mPosition;
    Vector2f mOrigin;
};

} // namespace oci
