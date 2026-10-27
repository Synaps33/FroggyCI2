#include "image_codec.h"

#define STBI_ONLY_JPEG
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_THREAD_LOCALS
#define STBI_ASSERT(x)
#include "../../third_party/stb_image.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace oci {
namespace resources {

namespace {

uint16_t rd16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

void put16(std::vector<char>& out, uint16_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
}

void put32(std::vector<char>& out, uint32_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
    out.push_back(static_cast<char>((v >> 16) & 0xFF));
    out.push_back(static_cast<char>((v >> 24) & 0xFF));
}

struct TgaInfo {
    int width = 0;
    int height = 0;
    int bpp = 0;
    int type = 0;
    int id_length = 0;
    int color_map_type = 0;
    int color_map_first = 0;
    int color_map_length = 0;
    int color_map_bits = 0;
    bool top_origin = false;
    size_t data_offset = 0;
    size_t bytes_per_pixel = 0;
};

bool ParseTga(const std::vector<char>& file, TgaInfo& info) {
    if(file.size() < 18)
        return false;
    const uint8_t* d = reinterpret_cast<const uint8_t*>(file.data());
    info.id_length = d[0];
    info.color_map_type = d[1];
    info.type = d[2];
    info.color_map_first = rd16(d + 3);
    info.color_map_length = rd16(d + 5);
    info.color_map_bits = d[7];
    info.width = rd16(d + 12);
    info.height = rd16(d + 14);
    info.bpp = d[16];
    info.top_origin = (d[17] & 0x20) != 0;
    if(info.width <= 0 || info.height <= 0)
        return false;
    if(info.type != 2 && info.type != 3 && info.type != 10 && info.type != 11)
        return false;
    if(info.bpp != 8 && info.bpp != 24 && info.bpp != 32)
        return false;
    info.bytes_per_pixel = static_cast<size_t>((info.bpp + 7) / 8);
    size_t color_map_size = 0;
    if(info.color_map_type != 0) {
        if(info.color_map_bits == 0)
            return false;
        color_map_size = static_cast<size_t>(info.color_map_length) * ((info.color_map_bits + 7) / 8);
    }
    info.data_offset = 18u + static_cast<size_t>(info.id_length) + color_map_size;
    size_t raw = static_cast<size_t>(info.width) * static_cast<size_t>(info.height) * info.bytes_per_pixel;
    if(info.type == 2 || info.type == 3) {
        if(info.data_offset + raw > file.size())
            return false;
    } else if(info.data_offset >= file.size()) {
        return false;
    }
    return true;
}

inline void tga_pixel_to_rgb(const uint8_t* p, int bpp, uint8_t& r, uint8_t& g, uint8_t& b, bool& opaque) {
    if(bpp == 8) {
        r = g = b = p[0];
        opaque = true;
    } else if(bpp == 24) {
        b = p[0];
        g = p[1];
        r = p[2];
        opaque = true;
    } else {
        b = p[0];
        g = p[1];
        r = p[2];
        opaque = p[3] >= 128;
    }
}

bool EncodeBmp24(const std::vector<uint8_t>& rgb, int width, int height, std::vector<char>& out) {
    if(width <= 0 || height <= 0)
        return false;
    const size_t row_size = (static_cast<size_t>(width) * 3u + 3u) & ~static_cast<size_t>(3u);
    const size_t image_size = row_size * static_cast<size_t>(height);
    const size_t file_size = 54u + image_size;
    if(file_size > 0x7FFFFFFFu)
        return false;

    out.clear();
    out.reserve(file_size);
    out.push_back('B');
    out.push_back('M');
    put32(out, static_cast<uint32_t>(file_size));
    put16(out, 0);
    put16(out, 0);
    put32(out, 54);
    put32(out, 40);
    put32(out, static_cast<uint32_t>(width));
    put32(out, static_cast<uint32_t>(height));
    put16(out, 1);
    put16(out, 24);
    put32(out, 0);
    put32(out, static_cast<uint32_t>(image_size));
    put32(out, 2835);
    put32(out, 2835);
    put32(out, 0);
    put32(out, 0);

    std::vector<char> row(row_size, 0);
    for(int y = height - 1; y >= 0; --y) {
        const uint8_t* src = &rgb[static_cast<size_t>(y) * static_cast<size_t>(width) * 3u];
        for(int x = 0; x < width; ++x) {
            row[static_cast<size_t>(x) * 3u + 0] = static_cast<char>(src[x * 3 + 2]);
            row[static_cast<size_t>(x) * 3u + 1] = static_cast<char>(src[x * 3 + 1]);
            row[static_cast<size_t>(x) * 3u + 2] = static_cast<char>(src[x * 3 + 0]);
        }
        out.insert(out.end(), row.begin(), row.end());
    }
    return true;
}

void NineSlice(const std::vector<uint8_t>& src_rgb, int sw, int sh,
               std::vector<uint8_t>& dst_rgb, int dw, int dh,
               int bl, int br, int bt, int bb) {
    dst_rgb.assign(static_cast<size_t>(dw) * static_cast<size_t>(dh) * 3u, 0);

    bl = std::clamp(bl, 0, sw / 2);
    br = std::clamp(br, 0, sw / 2);
    bt = std::clamp(bt, 0, sh / 2);
    bb = std::clamp(bb, 0, sh / 2);

    int src_mw = sw - bl - br;
    int src_mh = sh - bt - bb;
    int dst_mw = dw - bl - br;
    int dst_mh = dh - bt - bb;
    if(src_mw <= 0 || src_mh <= 0 || dst_mw <= 0 || dst_mh <= 0) {
        for(int dy = 0; dy < dh; ++dy) {
            int sy = dy * sh / dh;
            for(int dx = 0; dx < dw; ++dx) {
                int sx = dx * sw / dw;
                for(int c = 0; c < 3; ++c)
                    dst_rgb[(dy * dw + dx) * 3 + c] = src_rgb[(sy * sw + sx) * 3 + c];
            }
        }
        return;
    }

    for(int dy = 0; dy < dh; ++dy) {
        int sy;
        if(dy < bt)
            sy = dy;
        else if(dy >= dh - bb)
            sy = sh - bb + (dy - (dh - bb));
        else
            sy = bt + (dy - bt) * src_mh / dst_mh;

        for(int dx = 0; dx < dw; ++dx) {
            int sx;
            if(dx < bl)
                sx = dx;
            else if(dx >= dw - br)
                sx = sw - br + (dx - (dw - br));
            else
                sx = bl + (dx - bl) * src_mw / dst_mw;

            for(int c = 0; c < 3; ++c)
                dst_rgb[(dy * dw + dx) * 3 + c] = src_rgb[(sy * sw + sx) * 3 + c];
        }
    }
}

bool DecodeTgaFrame(const std::vector<char>& file, const TgaInfo& info,
                    int x, int y, int width, int height,
                    std::vector<char>& bmp,
                    int target_w, int target_h,
                    int bl, int br, int bt, int bb) {
    if(x < 0 || y < 0 || width <= 0 || height <= 0)
        return false;
    if(x + width > info.width || y + height > info.height)
        return false;

    std::vector<uint8_t> rgb(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
    const uint8_t* data = reinterpret_cast<const uint8_t*>(file.data());
    auto store = [&](int dx, int dy, const uint8_t* p) {
        uint8_t r, g, b;
        bool opaque;
        tga_pixel_to_rgb(p, info.bpp, r, g, b, opaque);
        if(!opaque) {
            r = g = b = 0;
        }
        size_t off = (static_cast<size_t>(dy) * static_cast<size_t>(width) + static_cast<size_t>(dx)) * 3u;
        rgb[off + 0] = r;
        rgb[off + 1] = g;
        rgb[off + 2] = b;
    };

    if(info.type == 2 || info.type == 3) {
        for(int dy = 0; dy < height; ++dy) {
            int screen_y = y + dy;
            int file_y = info.top_origin ? screen_y : (info.height - 1 - screen_y);
            const uint8_t* row = data + info.data_offset +
                (static_cast<size_t>(file_y) * static_cast<size_t>(info.width) + static_cast<size_t>(x)) * info.bytes_per_pixel;
            for(int dx = 0; dx < width; ++dx) {
                store(dx, dy, row + static_cast<size_t>(dx) * info.bytes_per_pixel);
            }
        }
    } else {
        size_t pos = info.data_offset;
        int total = info.width * info.height;
        int pixel_index = 0;
        while(pixel_index < total && pos < file.size()) {
            uint8_t packet = data[pos++];
            int count = (packet & 0x7F) + 1;
            if(packet & 0x80) {
                if(pos + info.bytes_per_pixel > file.size())
                    return false;
                const uint8_t* p = data + pos;
                pos += info.bytes_per_pixel;
                int sx = pixel_index % info.width;
                int file_y = pixel_index / info.width;
                int screen_y = info.top_origin ? file_y : (info.height - 1 - file_y);
                for(int n = 0; n < count && pixel_index < total; ++n, ++pixel_index, ++sx) {
                    if(sx >= info.width) {
                        sx = 0;
                        ++file_y;
                        screen_y = info.top_origin ? file_y : (info.height - 1 - file_y);
                    }
                    if(screen_y >= y && screen_y < y + height && sx >= x && sx < x + width)
                        store(sx - x, screen_y - y, p);
                }
            } else {
                for(int n = 0; n < count && pixel_index < total; ++n, ++pixel_index) {
                    if(pos + info.bytes_per_pixel > file.size())
                        return false;
                    int sx = pixel_index % info.width;
                    int file_y = pixel_index / info.width;
                    int screen_y = info.top_origin ? file_y : (info.height - 1 - file_y);
                    if(screen_y >= y && screen_y < y + height && sx >= x && sx < x + width)
                        store(sx - x, screen_y - y, data + pos);
                    pos += info.bytes_per_pixel;
                }
            }
        }
    }

    if(target_w > 0 && target_h > 0 && (target_w != width || target_h != height)) {
        std::vector<uint8_t> sliced;
        NineSlice(rgb, width, height, sliced, target_w, target_h, bl, br, bt, bb);
        return EncodeBmp24(sliced, target_w, target_h, bmp);
    }

    return EncodeBmp24(rgb, width, height, bmp);
}

bool DecodeJpegFrame(const std::vector<char>& file, int x, int y, int width, int height,
                     std::vector<char>& bmp, int target_w, int target_h) {
    int image_width = 0, image_height = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(file.data()),
        static_cast<int>(file.size()),
        &image_width, &image_height, &channels, 3);
    if(!pixels)
        return false;
    bool ok = false;
    if(x >= 0 && y >= 0 && width > 0 && height > 0 &&
       x + width <= image_width && y + height <= image_height) {
        std::vector<uint8_t> rgb(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
        for(int row = 0; row < height; ++row) {
            const unsigned char* src = pixels +
                (static_cast<size_t>(y + row) * static_cast<size_t>(image_width) + static_cast<size_t>(x)) * 3u;
            std::memcpy(&rgb[static_cast<size_t>(row) * static_cast<size_t>(width) * 3u],
                        src, static_cast<size_t>(width) * 3u);
        }

        if(target_w > 0 && target_h > 0 && (target_w != width || target_h != height)) {
            std::vector<uint8_t> scaled(static_cast<size_t>(target_w) * static_cast<size_t>(target_h) * 3u);
            for(int dy = 0; dy < target_h; ++dy) {
                int sy = dy * height / target_h;
                for(int dx = 0; dx < target_w; ++dx) {
                    int sx = dx * width / target_w;
                    for(int c = 0; c < 3; ++c)
                        scaled[(dy * target_w + dx) * 3 + c] = rgb[(sy * width + sx) * 3 + c];
                }
            }
            ok = EncodeBmp24(scaled, target_w, target_h, bmp);
        } else {
            ok = EncodeBmp24(rgb, width, height, bmp);
        }
    }
    stbi_image_free(pixels);
    return ok;
}

} // namespace

bool GetImageSize(const std::vector<char>& file, bool jpeg, int& width, int& height) {
    if(jpeg) {
        int channels = 0;
        return stbi_info_from_memory(
            reinterpret_cast<const unsigned char*>(file.data()),
            static_cast<int>(file.size()),
            &width, &height, &channels) != 0;
    }
    TgaInfo info;
    if(!ParseTga(file, info))
        return false;
    width = info.width;
    height = info.height;
    return true;
}

bool ExtractImageFrame(const std::vector<char>& file, bool jpeg,
                       int x, int y, int width, int height,
                       std::vector<char>& bmp,
                       int target_w, int target_h,
                       int border_l, int border_r,
                       int border_t, int border_b) {
    if(jpeg)
        return DecodeJpegFrame(file, x, y, width, height, bmp, target_w, target_h);
    TgaInfo info;
    if(!ParseTga(file, info))
        return false;
    return DecodeTgaFrame(file, info, x, y, width, height, bmp,
                          target_w, target_h, border_l, border_r, border_t, border_b);
}

} // namespace resources
} // namespace oci
