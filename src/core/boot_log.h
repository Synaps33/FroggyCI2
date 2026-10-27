#pragma once

namespace oci {
    // Minimal, crash-safe tracing of the core startup. Appends each line and
    // closes the file immediately so a hang/crash leaves a readable trace.
    void BootLog(const char* fmt, ...);
}
