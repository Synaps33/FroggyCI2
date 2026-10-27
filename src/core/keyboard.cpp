#include "keyboard.h"

namespace oci {

static bool s_keys[Keyboard::KeyCount] = { false };

bool Keyboard::isKeyPressed(Key key) {
    if(key >= 0 && key < KeyCount)
        return s_keys[key];
    return false;
}

void Keyboard::SetKeyPressed(Key key, bool pressed) {
    if(key >= 0 && key < KeyCount)
        s_keys[key] = pressed;
}

void Keyboard::Reset() {
    for(int i = 0; i < KeyCount; ++i)
        s_keys[i] = false;
}

} // namespace oci
