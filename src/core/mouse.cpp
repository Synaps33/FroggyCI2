#include "mouse.h"
#include "window.h"

namespace oci {

static int s_mouse_x = -100;
static int s_mouse_y = -100;
static bool s_mouse_buttons[Mouse::ButtonCount] = { false };

bool Mouse::isButtonPressed(Button button) {
    if(button >= 0 && button < ButtonCount)
        return s_mouse_buttons[button];
    return false;
}

Vector2i Mouse::getPosition(const Window& /*window*/) {
    return Vector2i(s_mouse_x, s_mouse_y);
}

void Mouse::setPosition(const Vector2i& position, const Window& /*window*/) {
    s_mouse_x = position.x;
    s_mouse_y = position.y;
}

void Mouse::SetPosition(int x, int y) {
    s_mouse_x = x;
    s_mouse_y = y;
}

void Mouse::SetButtonPressed(Button button, bool pressed) {
    if(button >= 0 && button < ButtonCount)
        s_mouse_buttons[button] = pressed;
}

} // namespace oci
