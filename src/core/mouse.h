#pragma once

#include "vector2.h"

namespace oci {

class Window;

class Mouse {
public:
    enum Button {
        Left = 0,
        Right = 1,
        Middle = 2,
        ButtonCount = 4
    };

    static bool isButtonPressed(Button button);
    static Vector2i getPosition(const Window& window);
    static void setPosition(const Vector2i& position, const Window& window);
    static void SetPosition(int x, int y);
    static void SetButtonPressed(Button button, bool pressed);
};

} // namespace oci
