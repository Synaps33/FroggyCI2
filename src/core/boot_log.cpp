#include "boot_log.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(SF2000) || defined(GB300)
#define OCI_BOOT_LOG_PATH "/mnt/sda1/oci_boot.log"
#endif

namespace oci {

void BootLog(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

#if defined(OCI_BOOT_LOG_PATH)
    FILE* f = fopen(OCI_BOOT_LOG_PATH, "a");
    if(f) {
        fwrite(buffer, 1, static_cast<size_t>(strlen(buffer)), f);
        fwrite("\n", 1, 1, f);
        fclose(f);
    }
#else
    static int enabled = -1;
    if(enabled < 0)
        enabled = std::getenv("OCI_BOOT_LOG") ? 1 : 0;
    if(enabled)
        std::fprintf(stderr, "OCI: %s\n", buffer);
#endif
}

namespace {
    struct CoreBootMarker {
        CoreBootMarker() { BootLog("core static init"); }
    };
    CoreBootMarker g_core_boot_marker;
} // namespace

} // namespace oci
