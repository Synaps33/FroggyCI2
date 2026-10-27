#pragma once

#include <cstdint>

namespace oci {

class Color {
public:
    Color() : r(0), g(0), b(0) {}
    Color(unsigned char r, unsigned char g, unsigned char b)
        : r(r), g(g), b(b) {}

    static const Color Black;
    static const Color White;

    uint16_t toRGB565() const noexcept {
        return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }

    unsigned char r, g, b;
};

} // namespace oci
