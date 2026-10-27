#pragma once

#include <cstddef>
#include <vector>

namespace oci {
namespace resources {

bool DecodeOggToWav(const std::vector<char>& file, std::vector<char>& wav);

} // namespace resources
} // namespace oci
