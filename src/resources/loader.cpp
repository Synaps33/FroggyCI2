#include "loader.h"
#include "image_codec.h"
#include "ogg_decoder.h"
#include "path_helper.h"

#include <core/boot_log.h>
#include <core/critical_error.h>

#include <tinyxml2.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace oci {
namespace resources {

namespace {

constexpr unsigned char NAME_XOR = 0xCC;

uint32_t rd32(const char* p) {
    const unsigned char* u = reinterpret_cast<const unsigned char*>(p);
    return static_cast<uint32_t>(u[0]) |
           (static_cast<uint32_t>(u[1]) << 8) |
           (static_cast<uint32_t>(u[2]) << 16) |
           (static_cast<uint32_t>(u[3]) << 24);
}

bool EndsWithNoCase(std::string_view value, std::string_view suffix) {
    if(value.size() < suffix.size())
        return false;
    for(std::size_t i = 0; i < suffix.size(); ++i) {
        char a = value[value.size() - suffix.size() + i];
        char b = suffix[i];
        if(std::tolower(static_cast<unsigned char>(a)) !=
           std::tolower(static_cast<unsigned char>(b)))
            return false;
    }
    return true;
}

std::string ToLower(std::string_view value) {
    std::string result(value);
    for(char& c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

bool WantsBiggerGameSprite(std::string_view name) {
    return name.rfind("gfx/fighter", 0) == 0;
}

std::string DecodeName(const char* data, std::size_t size) {
    std::string result;
    result.reserve(32);
    for(std::size_t i = 0; i < size; ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        if(c == NAME_XOR)
            break;
        char decoded = static_cast<char>(c ^ NAME_XOR);
        unsigned char u = static_cast<unsigned char>(decoded);
        if(u < 32 || u > 126)
            break;
        result.push_back(decoded);
    }
    return result;
}

bool ParseFrameIndex(std::string_view stem, std::string& base, int& frame) {
    std::size_t slash = stem.rfind('/');
    std::size_t underscore = stem.rfind('_');
    if(underscore == std::string_view::npos || underscore + 1 >= stem.size())
        return false;
    if(slash != std::string_view::npos && underscore < slash)
        return false;
    int value = 0;
    for(std::size_t i = underscore + 1; i < stem.size(); ++i) {
        char c = stem[i];
        if(c < '0' || c > '9')
            return false;
        value = value * 10 + (c - '0');
        if(value > 1000000)
            return false;
    }
    base.assign(stem.substr(0, underscore));
    frame = value;
    return true;
}

std::string StemOf(std::string_view name) {
    std::size_t slash = name.rfind('/');
    std::size_t dot = name.rfind('.');
    std::size_t start = (slash == std::string_view::npos) ? 0 : slash + 1;
    std::size_t end = (dot == std::string_view::npos || dot < start) ? name.size() : dot;
    return std::string(name.substr(start, end - start));
}

const char* SoundAlias(std::string_view stem, std::string& storage) {
    struct Alias { const char* from; const char* to; };
    static const Alias aliases[] = {
        {"appetite", "chomp"},
        {"bang", "crack"},
        {"bang_asteroid", "crack"},
        {"bang_ufo", "chomp"},
        {"bonus", "fanfar1"},
        {"chicken1", "bonewah"},
        {"chicken2", "bonewah"},
        {"chicken3", "bonewah"},
        {"chicken4", "bonewah"},
        {"coin", "coinbnce"},
        {"egg_end", "crack"},
        {"egg_start", "toon40"},
        {"engine_start", "gasengstart"},
        {"gun1", "laser"},
        {"gun3", "laser"},
        {"leg", "chomp"},
        {"missile_start", "fx39start-11k"},
        {"reverse", "rdfx31"},
        {"ufo", "hb12"},
        {"ufo_fire", "laserLow"},
        {"update", "fanfare"},
        {nullptr, nullptr}
    };
    for(int i = 0; aliases[i].from; ++i) {
        if(stem == aliases[i].from) {
            storage.assign(aliases[i].to);
            return storage.c_str();
        }
    }
    storage.assign(stem);
    return storage.c_str();
}

} // namespace

class ResourceLoader::Impl {
public:
    explicit Impl(std::string_view preferred) : mPreferred(ToLower(preferred)) {
        OpenArchives();
    }

    std::vector<char> GetData(std::string_view resource_name) {
        std::vector<char> result;
        if(FindData(resource_name, result))
            return result;
        BootLog("resource MISSING: %.*s", static_cast<int>(resource_name.size()), resource_name.data());
        CriticalError("Cannot find file \"", resource_name, '"');
    }

    bool TryGetData(std::string_view resource_name, std::vector<char>& result) {
        return FindData(resource_name, result);
    }

    bool IsFullVersion() const {
        return !mFull.empty();
    }

    std::string PrimaryArchiveName() const {
        const std::string* path = nullptr;
        if(!mFull.empty())
            path = &mFull.front()->path;
        else if(!mLegacy.empty())
            path = &mLegacy.front()->path;
        if(!path)
            return std::string();
        std::size_t slash = path->find_last_of("/\\");
        return slash == std::string::npos ? *path : path->substr(slash + 1);
    }

private:
    struct Entry {
        uint32_t position = 0;
        uint32_t size = 0;
    };

    struct Archive {
        std::string path;
        mutable std::ifstream file;
        std::unordered_map<std::string, Entry> toc;
        uint64_t file_size = 0;
        bool full = false;
    };

    std::string mPreferred;

    std::vector<std::unique_ptr<Archive>> mFull;
    std::vector<std::unique_ptr<Archive>> mLegacy;
    std::string mCachedAtlasName;
    std::vector<char> mCachedAtlas;
    struct LayoutSize { int width; int height; };
    std::unordered_map<std::string, LayoutSize> mLayout;
    bool mLayoutLoaded = false;

    static std::vector<std::string> CandidateNames() {
        return {
            "CI2rm.dat",
            "CI2rm.dat.1250",
            "CI2rm.dat.1252",
            "CI2rm.dat.language.v2",
            "CI2rmXmas.dat",
            "CI2rmXmas.dat.1250",
            "CI2rmXmas.dat.1252",
            "CI2rmXmas.dat.language.v2",
            "ChickenInvaders2demo.dat",
            "ChickenInvaders2.dat"
        };
    }

    static std::vector<std::string> SplitList(const char* value) {
        std::vector<std::string> result;
        std::string current;
        for(const char* p = value; *p; ++p) {
            if(*p == ':' || *p == ';') {
                if(!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current.push_back(*p);
            }
        }
        if(!current.empty())
            result.push_back(current);
        return result;
    }

    void OpenArchives() {
        std::vector<std::string> paths;
        const char* env = std::getenv("OCI_DAT");
        if(env && *env)
            paths = SplitList(env);
        else {
            for(const std::string& name : CandidateNames()) {
                const std::string lower = ToLower(name);
                if(mPreferred.find("xmas") != std::string::npos) {
                    if(lower.find("xmas") == std::string::npos)
                        continue;
                } else if(mPreferred.find("ci2rm.dat") != std::string::npos ||
                          mPreferred.find("ci2rm") != std::string::npos) {
                    if(lower.find("xmas") != std::string::npos)
                        continue;
                }
                paths.push_back(ResolvePath(name));
            }
        }

        std::vector<std::string> added;
        for(const std::string& path : paths) {
            if(std::find(added.begin(), added.end(), path) != added.end())
                continue;
            if(AddArchive(path))
                added.push_back(path);
        }

        if(mFull.empty() && mLegacy.empty()) {
            const std::string path = paths.empty() ? std::string("ChickenInvaders2.dat") : paths.front();
            BootLog("DAT open FAILED: %s", path.c_str());
            std::cerr << "FATAL ERROR: Could not open resource file \"" << path << "\"" << std::endl;
            std::exit(1);
        }
        BootLog("DAT archives: full=%zu legacy=%zu", mFull.size(), mLegacy.size());
    }

    bool AddArchive(const std::string& path) {
        std::unique_ptr<Archive> archive(new Archive);
        archive->path = path;
        archive->file.open(path.c_str(), std::ios_base::in | std::ios_base::binary);
        if(!archive->file) {
            BootLog("DAT open skipped: %s", path.c_str());
            return false;
        }
        archive->file.seekg(0, std::ios::end);
        std::streamoff end = archive->file.tellg();
        if(end <= 0) {
            BootLog("DAT empty: %s", path.c_str());
            return false;
        }
        archive->file_size = static_cast<uint64_t>(end);
        archive->file.seekg(0, std::ios::beg);
        if(!ReadTOC(*archive))
            return false;
        BootLog("DAT %s: %s, %zu entries", path.c_str(),
                archive->full ? "full v2" : "legacy v1", archive->toc.size());
        if(archive->full)
            mFull.push_back(std::move(archive));
        else
            mLegacy.push_back(std::move(archive));
        return true;
    }

    bool ReadTOC(Archive& archive) {
        char header[8];
        if(!archive.file.read(header, sizeof(header)))
            return false;
        if(std::memcmp(header, "UVE WAD ", 8) == 0) {
            BootLog("DAT unsupported UVE WAD: %s", archive.path.c_str());
            return false;
        }
        const uint32_t first = rd32(header);
        const uint32_t second = rd32(header + 4);
        const uint64_t v2_size = 8ull + static_cast<uint64_t>(second) * 92ull;
        if(first == 2 && second > 0 && second < 1000000 && v2_size <= archive.file_size) {
            std::vector<char> toc(static_cast<std::size_t>(v2_size - 8));
            archive.file.seekg(8, std::ios::beg);
            if(!archive.file.read(toc.data(), static_cast<std::streamsize>(toc.size())))
                return false;
            for(uint32_t i = 0; i < second; ++i) {
                const char* record = toc.data() + static_cast<std::size_t>(i) * 92;
                std::string name = DecodeName(record, 80);
                const uint32_t position = rd32(record + 80);
                const uint32_t size = rd32(record + 84);
                if(name.empty())
                    continue;
                if(static_cast<uint64_t>(position) + size > archive.file_size)
                    return false;
                archive.toc.emplace(std::move(name), Entry{position, size});
            }
            archive.full = true;
            return !archive.toc.empty();
        }

        const uint32_t count = first;
        const uint64_t v1_size = 4ull + static_cast<uint64_t>(count) * 88ull;
        if(count == 0 || count >= 1000000 || v1_size > archive.file_size)
            return false;
        std::vector<char> toc(static_cast<std::size_t>(v1_size - 4));
        archive.file.seekg(4, std::ios::beg);
        if(!archive.file.read(toc.data(), static_cast<std::streamsize>(toc.size())))
            return false;
        for(uint32_t i = 0; i < count; ++i) {
            const char* record = toc.data() + static_cast<std::size_t>(i) * 88;
            std::string name = DecodeName(record, 80);
            const uint32_t position = rd32(record + 80);
            const uint32_t size = rd32(record + 84);
            if(name.empty())
                continue;
            if(static_cast<uint64_t>(position) + size > archive.file_size)
                return false;
            archive.toc.emplace(std::move(name), Entry{position, size});
        }
        archive.full = false;
        return !archive.toc.empty();
    }

    bool ReadEntry(const Archive& archive, const Entry& entry, std::vector<char>& out) const {
        out.resize(entry.size);
        if(entry.size == 0)
            return true;
        archive.file.clear();
        archive.file.seekg(static_cast<std::streamoff>(entry.position), std::ios::beg);
        if(!archive.file.read(out.data(), static_cast<std::streamsize>(entry.size)))
            return false;
        return true;
    }

    bool FindExact(std::string_view name, std::vector<char>& out, bool full_only) const {
        const std::string key(name);
        for(const auto& archive : mFull) {
            auto it = archive->toc.find(key);
            if(it != archive->toc.end())
                return ReadEntry(*archive, it->second, out);
        }
        if(!full_only) {
            for(const auto& archive : mLegacy) {
                auto it = archive->toc.find(key);
                if(it != archive->toc.end())
                    return ReadEntry(*archive, it->second, out);
            }
        }
        return false;
    }

    bool FindFull(std::string_view name, std::vector<char>& out) const {
        return FindExact(name, out, true);
    }

    bool IsXmasMode() const {
        if(mPreferred.find("xmas") != std::string::npos)
            return true;
        if(!mFull.empty() && ToLower(mFull.front()->path).find("xmas") != std::string::npos)
            return true;
        return false;
    }

    bool FindData(std::string_view name, std::vector<char>& out) {
        if(mFull.empty() &&
           (name == "gfx/hud-top_0.bmp" || name == "gfx/hud-bottom_0.bmp"))
            return FindExact("gfx/hud_0.bmp", out, false);

        std::string external;
        if(name == "gfx/logo2-small_0.bmp" && !IsXmasMode())
            external = "res/images/logo-nextwave.bmp";
        else if(name.rfind("system/score_", 0) == 0)
            external = "res/images/" + std::string(name.substr(7));
        else if(name.rfind("gfx/menuitem_", 0) == 0 && !IsXmasMode())
            external = "res/images/" + std::string(name.substr(4));
        if(!external.empty()) {
            const std::string path = ResolvePath(external);
            std::ifstream file(path.c_str(), std::ios_base::in | std::ios_base::binary);
            if(file) {
                file.seekg(0, std::ios::end);
                const std::streamoff size = file.tellg();
                if(size > 0) {
                    file.seekg(0, std::ios::beg);
                    out.resize(static_cast<std::size_t>(size));
                    if(file.read(out.data(), size))
                        return true;
                }
            }
        }
        if(FindExact(name, out, true))
            return true;
        if(!mFull.empty()) {
            if(EndsWithNoCase(name, ".bmp") && ResolveImageAlias(name, out))
                return true;
            if(EndsWithNoCase(name, ".wav") && ResolveSoundAlias(name, out))
                return true;
        }
        if(mFull.empty())
            return FindExact(name, out, false);
        return false;
    }

    bool LoadAtlas(const std::string& atlas, std::vector<char>& data) const {
        return FindFull(atlas, data);
    }

    void LoadLayout() {
        if(mLayoutLoaded)
            return;
        mLayoutLoaded = true;
        const std::string path = ResolvePath("res/layout.xml");
        tinyxml2::XMLDocument doc;
        if(doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
            BootLog("layout: cannot load %s, using 0.5 fallback", path.c_str());
            return;
        }
        const tinyxml2::XMLElement* root = doc.FirstChildElement("layout");
        if(!root)
            return;
        for(const tinyxml2::XMLElement* image = root->FirstChildElement("image");
            image; image = image->NextSiblingElement("image")) {
            const char* name = image->Attribute("name");
            if(!name)
                continue;
            LayoutSize size{0, 0};
            image->QueryIntAttribute("width", &size.width);
            image->QueryIntAttribute("height", &size.height);
            if(size.width > 0 && size.height > 0)
                mLayout.emplace(name, size);
        }
        BootLog("layout: %zu entries", mLayout.size());
    }

    bool GetLayoutSize(std::string_view name, int& width, int& height) {
        LoadLayout();
        auto it = mLayout.find(std::string(name));
        if(it == mLayout.end())
            return false;
        width = it->second.width;
        height = it->second.height;
        return true;
    }

    bool ResolveImageAlias(std::string_view name, std::vector<char>& out) {
        std::string stem(name.substr(0, name.size() - 4));
        std::string base = stem;
        int frame = 0;
        ParseFrameIndex(stem, base, frame);

        struct Ref {
            std::string atlas;
            int frame;
        };
        std::vector<Ref> refs;
        auto add = [&refs](std::string atlas, int index) {
            refs.push_back(Ref{std::move(atlas), index});
        };

        if(base == "gfx/heart") {
            add("gfx/gui/game/hud.tga", 2);
            add("xmas-edition/gfx/gui/game/hud.tga", 2);
        } else if(base == "gfx/iconbomb") {
            add("gfx/gui/game/hud.tga", 3);
            add("xmas-edition/gfx/gui/game/hud.tga", 3);
        } else if(base == "gfx/hud") {
            add("gfx/gui/game/hud.tga", 0);
            add("xmas-edition/gfx/gui/game/hud.tga", 0);
        } else if(base == "gfx/menuitem") {
            add("gfx/gui/game/button.tga", frame % 3);
            add("xmas-edition/gfx/gui/game/button.tga", frame % 3);
        } else if(base == "gfx/logo2-small") {
            add("gfx/logo.tga", 0);
            add("xmas-edition/gfx/logo.tga", 0);
        } else if(base == "gfx/fork") {
            add("gfx/system/crosshair.tga", 0);
        } else if(base == "gfx/hud-bottom") {
            add("gfx/gui/game/hud.tga", 0);
            add("xmas-edition/gfx/gui/game/hud.tga", 0);
        } else if(base == "gfx/hud-top") {
            add("gfx/gui/game/hud.tga", 0);
            add("xmas-edition/gfx/gui/game/hud.tga", 0);
        } else if(base == "gfx/salmonsky-tile") {
            add("gfx/ci2rm/salmonsky-tile.jpg", 0);
            add("xmas-edition/gfx/ci2rm/salmonsky-tile.jpg", 0);
        } else if(base == "system/myriad9pt") {
            add("gfx/font/1250/font-small-1.tga", frame);
            add("gfx/font/1252/font-small-1.tga", frame);
            add("gfx/font/1250/font-small-2.tga", frame);
            add("gfx/font/1252/font-small-1.tga", frame);
            add("xmas-edition/gfx/font/1250/font-small-1.tga", frame);
        } else if(base == "system/alphabet-s") {
            add("gfx/font/1250/font-medium-1.tga", frame);
            add("gfx/font/1252/font-medium-1.tga", frame);
            add("gfx/font/1250/font-medium-2.tga", frame);
            add("gfx/font/1250/font-small-1.tga", frame);
            add("xmas-edition/gfx/font/1250/font-medium-1.tga", frame);
            add("xmas-edition/gfx/font/1250/font-medium-2.tga", frame);
        } else if(base == "system/alphabet") {
            add("gfx/font/1250/font-large-1.tga", frame);
            add("gfx/font/1252/font-large-1.tga", frame);
            add("gfx/font/1250/font-large-2.tga", frame);
            add("gfx/font/1250/font-large-3.tga", frame);
            add("gfx/font/1250/font-large-4.tga", frame);
            add("gfx/font/1250/font-large-5.tga", frame);
            add("gfx/font/1250/font-medium-1.tga", frame);
            add("xmas-edition/gfx/font/1250/font-large-1.tga", frame);
            add("xmas-edition/gfx/font/1250/font-large-2.tga", frame);
            add("xmas-edition/gfx/font/1250/font-large-3.tga", frame);
            add("xmas-edition/gfx/font/1250/font-large-4.tga", frame);
            add("xmas-edition/gfx/font/1250/font-large-5.tga", frame);
        } else if(base.compare(0, 4, "gfx/") == 0) {
            const std::string rest = base.substr(4);
            add("gfx/ci2rm/" + rest + ".tga", frame);
            add("gfx/ci2rm/" + rest + ".jpg", frame);
            add("gfx/gui/game/" + rest + ".tga", frame);
            add("gfx/gui/game/" + rest + ".jpg", frame);
            add("gfx/system/" + rest + ".tga", frame);
            add("gfx/" + rest + ".tga", frame);
            add("gfx/" + rest + ".jpg", frame);
            add("xmas-edition/gfx/ci2rm/" + rest + ".tga", frame);
            add("xmas-edition/gfx/gui/game/" + rest + ".tga", frame);
            add("xmas-edition/gfx/" + rest + ".tga", frame);
            add("xmas-edition/gfx/" + rest + ".jpg", frame);
        } else if(base.compare(0, 7, "system/") == 0) {
            const std::string rest = base.substr(7);
            add("gfx/system/" + rest + ".tga", frame);
            add("gfx/" + rest + ".tga", frame);
        } else {
            return false;
        }

        for(const Ref& ref : refs) {
            if(ExtractAtlasFrame(ref.atlas, ref.frame, name, out))
                return true;
        }
        return false;
    }

    bool ExtractAtlasFrame(const std::string& atlas, int frame,
                           std::string_view request_name, std::vector<char>& out) {
        static constexpr std::size_t CACHE_LIMIT = 2u * 1024u * 1024u;
        std::vector<char> local;
        const std::vector<char>* file = nullptr;
        if(!mCachedAtlasName.empty() && mCachedAtlasName == atlas && !mCachedAtlas.empty()) {
            file = &mCachedAtlas;
        } else {
            if(!LoadAtlas(atlas, local))
                return false;
            if(local.size() <= CACHE_LIMIT) {
                mCachedAtlasName = atlas;
                mCachedAtlas.swap(local);
                file = &mCachedAtlas;
            } else {
                mCachedAtlasName.clear();
                mCachedAtlas.clear();
                file = &local;
            }
        }
        const bool jpeg = EndsWithNoCase(atlas, ".jpg") || EndsWithNoCase(atlas, ".jpeg");

        int image_width = 0, image_height = 0;
        if(!GetImageSize(*file, jpeg, image_width, image_height))
            return false;

        int x = 0, y = 0, width = image_width, height = image_height;
        std::vector<char> preload;
        if(FindFull(atlas + ".ini.preload", preload) && preload.size() >= 20) {
            const uint32_t count = rd32(preload.data() + 16);
            if(frame < 0 || static_cast<uint32_t>(frame) >= count)
                return false;
            if(20ull + static_cast<uint64_t>(count) * 24ull > preload.size())
                return false;
            const char* record = preload.data() + 20 + static_cast<std::size_t>(frame) * 24;
            x = static_cast<int>(rd32(record));
            y = static_cast<int>(rd32(record + 4));
            width = static_cast<int>(rd32(record + 8));
            height = static_cast<int>(rd32(record + 12));
        } else if(frame != 0) {
            return false;
        }

        if(x < 0 || y < 0 || width <= 0 || height <= 0)
            return false;
        if(x + width > image_width)
            width = image_width - x;
        if(y + height > image_height)
            height = image_height - y;
        if(width <= 0 || height <= 0)
            return false;
        int target_w = 0, target_h = 0;
        int bl = 0, br = 0, bt = 0, bb = 0;
        if(atlas.find("button.tga") != std::string::npos) {
            target_w = 208;
            target_h = 40;
            bl = 15;
            br = 15;
            bt = 10;
            bb = 10;
        } else if(request_name.rfind("gfx/hud-bottom", 0) == 0) {
            target_w = 140;
            target_h = 48;
        } else if(request_name.rfind("gfx/hud-top", 0) == 0) {
            target_w = 195;
            target_h = 58;
        } else if(atlas.find("salmonsky-tile.jpg") != std::string::npos) {
            target_w = 640;
            target_h = 480;
        } else {
            if(!GetLayoutSize(request_name, target_w, target_h)) {
                target_w = width > 1 ? width / 2 : 1;
                target_h = height > 1 ? height / 2 : 1;
            }
            if(WantsBiggerGameSprite(request_name)) {
                target_w = (target_w * 140 + 50) / 100;
                target_h = (target_h * 140 + 50) / 100;
            } else if(request_name.rfind("gfx/chicken-", 0) == 0 &&
                      request_name.rfind("gfx/chicken-leg", 0) != 0 &&
                      request_name.rfind("gfx/chicken-body", 0) != 0 &&
                      request_name.rfind("gfx/chicken-part", 0) != 0 &&
                      request_name.rfind("gfx/chicken-roast", 0) != 0 &&
                      request_name.rfind("gfx/chicken_roast", 0) != 0) {
                target_w = (target_w * 88 + 50) / 100;
                target_h = (target_h * 88 + 50) / 100;
            }
            if(request_name.rfind("system/alphabet-s_", 0) == 0) {
                target_w = (target_w * 125 + 50) / 100;
                target_h = (target_h * 125 + 50) / 100;
            }

            if(request_name.rfind("gfx/heart_", 0) == 0) {
                target_w = (target_w * 200 + 50) / 100;
                target_h = (target_h * 200 + 50) / 100;
            } else if(request_name.rfind("gfx/iconbomb_", 0) == 0) {
                target_w = (target_w * 120 + 50) / 100;
                target_h = (target_h * 120 + 50) / 100;
            }
            if(request_name.rfind("gfx/egg_0", 0) == 0) {
                target_w = (target_w * 195 + 50) / 100;
                target_h = (target_h * 195 + 50) / 100;
            }
            if(request_name.rfind("gfx/crystal2_", 0) == 0) {
                target_w = (target_w * 72 + 50) / 100;
                target_h = (target_h * 130 + 50) / 100;
            } else if((request_name.rfind("gfx/zigzag_", 0) == 0 ||
                       request_name.rfind("gfx/zigzag-small_", 0) == 0 ||
                       request_name.rfind("gfx/zigzag-big_", 0) == 0) &&
                      request_name.rfind("gfx/zigzag-purple", 0) != 0) {
                target_w = (target_w * 85 + 50) / 100;
                target_h = (target_h * 130 + 50) / 100;
            }
        }
        return ExtractImageFrame(*file, jpeg, x, y, width, height, out,
                                 target_w, target_h, bl, br, bt, bb);
    }

    bool ResolveSoundAlias(std::string_view name, std::vector<char>& out) {
        const std::string stem = StemOf(name);
        std::string storage;
        const char* target = SoundAlias(stem, storage);
        const std::string candidates[] = {
            std::string("sfx/ci2rm/") + target + ".ogg",
            std::string("sfx/") + target + ".ogg",
            std::string("xmas-edition/sfx/ci2rm/") + target + ".ogg"
        };
        for(const std::string& candidate : candidates) {
            std::vector<char> file;
            if(!FindFull(candidate, file))
                continue;
            if(!DecodeOggToWav(file, out))
                return false;
            return true;
        }
        return false;
    }
};

ResourceLoader ResourceLoader::mInstance;

ResourceLoader::ResourceLoader() = default;

std::vector<char> ResourceLoader::GetData(std::string_view resource_name) {
    if(!mImpl)
        mImpl.reset(new Impl(mPreferredDat));
    return mImpl->GetData(resource_name);
}

bool ResourceLoader::TryGetData(std::string_view resource_name, std::vector<char>& out) {
    if(!mImpl)
        mImpl.reset(new Impl(mPreferredDat));
    return mImpl->TryGetData(resource_name, out);
}

void ResourceLoader::SetPreferredDat(std::string_view path) {
    mPreferredDat.assign(path);
    mImpl.reset();
}

bool ResourceLoader::IsFullVersion() {
    if(!mImpl)
        mImpl.reset(new Impl(mPreferredDat));
    return mImpl->IsFullVersion();
}

std::string ResourceLoader::PrimaryArchiveName() {
    if(!mImpl)
        mImpl.reset(new Impl(mPreferredDat));
    return mImpl->PrimaryArchiveName();
}

} // namespace resources
} // namespace oci
