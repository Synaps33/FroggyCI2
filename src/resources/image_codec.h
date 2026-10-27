#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oci {
namespace resources {

bool GetImageSize(const std::vector<char>& file, bool jpeg, int& width, int& height);
bool ExtractImageFrame(const std::vector<char>& file, bool jpeg,
                       int x, int y, int width, int height,
                       std::vector<char>& bmp,
                       int target_w = 0, int target_h = 0,
                       int border_l = 0, int border_r = 0,
                       int border_t = 0, int border_b = 0);

} // namespace resources
} // namespace oci
