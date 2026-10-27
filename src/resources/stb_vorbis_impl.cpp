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
#endif

#define STB_VORBIS_NO_STDIO
#include "../../third_party/stb_vorbis.c"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
