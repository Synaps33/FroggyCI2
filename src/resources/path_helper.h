#pragma once

#include <string>

namespace oci {
    void SetBaseDirectory(const char* path);
    const std::string& GetBaseDirectory();
    std::string ResolvePath(const std::string& rel);
}
