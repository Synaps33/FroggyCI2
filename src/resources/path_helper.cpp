#include "path_helper.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace oci {

static std::string s_base_dir = ".";

static bool file_exists(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if(f) {
        fclose(f);
        return true;
    }
    return false;
}

void SetBaseDirectory(const char* path) {
    if(!path || !*path)
        return;
    s_base_dir = path;
    // Strip trailing slash if present
    while(s_base_dir.length() > 1 && (s_base_dir.back() == '/' || s_base_dir.back() == '\\')) {
        s_base_dir.pop_back();
    }
}

const std::string& GetBaseDirectory() {
    return s_base_dir;
}

std::string ResolvePath(const std::string& rel) {
    if(rel.empty())
        return rel;
    if(rel[0] == '/')
        return rel;
    if(s_base_dir != "." && !s_base_dir.empty()) {
        std::string cand = s_base_dir + "/" + rel;
        if(file_exists(cand))
            return cand;
    }
    if(file_exists(rel))
        return rel;
    // Check common SF2000 / GB300 paths
    const char* fallbacks[] = {
        "ROMS/chicken",
        "ROMS/oci",
        "ROMS",
        "/mnt/sda1/ROMS/chicken",
        "/mnt/sda1/ROMS",
        "/mnt/sda1",
        nullptr
    };
    for(int i = 0; fallbacks[i]; i++) {
        std::string cand = std::string(fallbacks[i]) + "/" + rel;
        if(file_exists(cand))
            return cand;
    }
    return rel;
}

} // namespace oci
