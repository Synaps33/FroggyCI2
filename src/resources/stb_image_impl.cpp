#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#define STBI_ONLY_JPEG
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_THREAD_LOCALS
#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
