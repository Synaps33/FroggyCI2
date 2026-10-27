#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#if __cplusplus < 202002L
#ifndef unlikely
#define unlikely
#endif
#ifndef likely
#define likely
#endif

namespace std {
namespace numbers {
    template<typename T = double>
    constexpr T pi_v = static_cast<T>(3.1415926535897932384626433832795);
    constexpr double pi = 3.1415926535897932384626433832795;
}

#if !defined(_GLIBCXX_HAVE_BROKEN_VSWPRINTF) && !defined(__GLIBCXX__)
inline string to_string(int val) {
    char b[32];
    snprintf(b, sizeof(b), "%d", val);
    return string(b);
}
#endif

}
#endif
