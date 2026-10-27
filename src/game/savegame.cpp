#include "savegame.h"

#include <core/boot_log.h>
#include <resources/path_helper.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace oci {
namespace SaveGame {

namespace {

constexpr uint32_t MAGIC = 0x5349434Fu;   // "OCIS"
constexpr uint32_t VERSION = 1;

const char* SavePath() {
    static std::string path;
    if(path.empty())
        path = ResolvePath("continue.sav");
    return path.c_str();
}

uint32_t Checksum(const uint32_t* words, int count) {
    uint32_t sum = 0x9E3779B9u;
    for(int i = 0; i < count; ++i) {
        sum ^= words[i] + 0x9E3779B9u + (sum << 6) + (sum >> 2);
    }
    return sum;
}

bool s_dirty = false;

} // namespace

bool Load(SaveData& out) {
    out = SaveData();
    FILE* f = std::fopen(SavePath(), "rb");
    if(!f)
        return false;

    uint32_t words[10];
    size_t got = std::fread(words, sizeof(uint32_t), 10, f);
    std::fclose(f);
    if(got != 10)
        return false;

    if(words[0] != MAGIC || words[1] != VERSION)
        return false;
    if(Checksum(words, 9) != words[9])
        return false;

    out.valid = words[2] != 0;
    out.score = static_cast<int>(static_cast<int32_t>(words[3]));
    out.lives = static_cast<int>(static_cast<int32_t>(words[4]));
    out.missiles = static_cast<int>(static_cast<int32_t>(words[5]));
    out.gun_type = static_cast<int>(static_cast<int32_t>(words[6]));
    out.gun_level = static_cast<int>(static_cast<int32_t>(words[7]));
    out.level = static_cast<int>(static_cast<int32_t>(words[8]));

    if(out.score < 0) out.score = 0;
    if(out.lives < 0) out.lives = 0;
    if(out.lives > 99) out.lives = 99;
    if(out.missiles < 0) out.missiles = 0;
    if(out.missiles > 99) out.missiles = 99;
    if(out.gun_type < 0 || out.gun_type >= 3) out.gun_type = 0;
    if(out.gun_level < 1) out.gun_level = 1;
    if(out.gun_level > 99) out.gun_level = 99;
    if(out.level < 1 || out.level > 120) out.valid = false;

    return out.valid;
}

bool Exists() {
    SaveData data;
    return Load(data) && data.valid;
}

void Write(const SaveData& data) {
    uint32_t words[10];
    words[0] = MAGIC;
    words[1] = VERSION;
    words[2] = data.valid ? 1u : 0u;
    words[3] = static_cast<uint32_t>(data.score);
    words[4] = static_cast<uint32_t>(data.lives);
    words[5] = static_cast<uint32_t>(data.missiles);
    words[6] = static_cast<uint32_t>(data.gun_type);
    words[7] = static_cast<uint32_t>(data.gun_level);
    words[8] = static_cast<uint32_t>(data.level);
    words[9] = Checksum(words, 9);

    FILE* f = std::fopen(SavePath(), "wb");
    if(!f) {
        BootLog("save: cannot open %s", SavePath());
        return;
    }
    std::fwrite(words, sizeof(uint32_t), 10, f);
    std::fclose(f);
    s_dirty = false;
}

void Invalidate() {
    SaveData data;
    data.valid = false;
    Write(data);
    s_dirty = false;
}

void MarkDirty() {
    s_dirty = true;
}

bool IsDirty() {
    return s_dirty;
}

void ClearDirty() {
    s_dirty = false;
}

} // namespace SaveGame
} // namespace oci
