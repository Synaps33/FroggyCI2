#pragma once

#include <cstdlib>
#include <iostream>

namespace oci {

inline void PrintError() {
    std::cerr << std::endl;
}

template <typename T, typename... MessageArgs>
inline void PrintError(const T& first, const MessageArgs&... args) {
    std::cerr << first;
    PrintError(args...);
}

template <typename... MessageArgs>
[[noreturn]] inline void CriticalError(const MessageArgs&... args) {
    std::cerr << "CRITICAL ERROR: ";
    PrintError(args...);
    std::abort();
}

} // namespace oci
