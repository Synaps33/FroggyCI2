#pragma once

namespace oci {

class Keyboard {
public:
    enum Key {
        Left = 0,
        Right = 1,
        Up = 2,
        Down = 3,
        Return = 4,
        Escape = 5,
        Space = 6,
        Fire = 7,
        KeyCount = 16
    };

    static bool isKeyPressed(Key key);
    static void SetKeyPressed(Key key, bool pressed);
    static void Reset();
};

} // namespace oci
