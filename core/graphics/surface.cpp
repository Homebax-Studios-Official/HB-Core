// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "graphics/surface.hpp"
#include "graphics/bitmap_font.hpp"
#include "base/strings.hpp"
#include <algorithm>
#include <cmath>

namespace HB {

IntRect IntRect::intersect(const IntRect& o) const {
    return {std::max(x0, o.x0), std::max(y0, o.y0), std::min(x1, o.x1), std::min(y1, o.y1)};
}

Surface::Surface(int width, int height) { resize(width, height); }

void Surface::resize(int width, int height) {
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    pixels_.assign(static_cast<size_t>(width_) * height_ * 4, 255);
    clips_.clear();
}

void Surface::clear(Color c) {
    for (size_t i = 0; i < pixels_.size(); i += 4) { pixels_[i] = c.r; pixels_[i + 1] = c.g; pixels_[i + 2] = c.b; pixels_[i + 3] = 255; }
}

IntRect Surface::currentClip() const {
    IntRect r{0, 0, width_, height_};
    if (!clips_.empty()) r = r.intersect(clips_.back());
    return r;
}

void Surface::pushClip(float x, float y, float w, float h) {
    IntRect r{static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), static_cast<int>(std::lround(x + w)), static_cast<int>(std::lround(y + h))};
    clips_.push_back(r.intersect(currentClip()));
}

void Surface::popClip() { if (!clips_.empty()) clips_.pop_back(); }

Color Surface::pixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return {0, 0, 0, 0};
    const uint8_t* p = &pixels_[(static_cast<size_t>(y) * width_ + x) * 4];
    return {p[0], p[1], p[2], p[3]};
}

void Surface::blend(int x, int y, Color c, float coverage) {
    float a = (c.a / 255.0f) * coverage;
    if (a <= 0) return;
    uint8_t* p = &pixels_[(static_cast<size_t>(y) * width_ + x) * 4];
    if (a >= 0.999f) { p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = 255; return; }
    p[0] = static_cast<uint8_t>(std::lround(c.r * a + p[0] * (1 - a)));
    p[1] = static_cast<uint8_t>(std::lround(c.g * a + p[1] * (1 - a)));
    p[2] = static_cast<uint8_t>(std::lround(c.b * a + p[2] * (1 - a)));
    p[3] = 255;
}

void Surface::fillRect(float x, float y, float w, float h, Color c) {
    if (w <= 0 || h <= 0 || c.a == 0) return;
    int x0 = static_cast<int>(std::lround(x)), x1 = static_cast<int>(std::lround(x + w));
    int y0 = static_cast<int>(std::lround(y)), y1 = static_cast<int>(std::lround(y + h));
    if (x1 == x0) x1 = x0 + 1;
    if (y1 == y0) y1 = y0 + 1;
    IntRect r = IntRect{x0, y0, x1, y1}.intersect(currentClip());
    if (r.empty()) return;
    for (int py = r.y0; py < r.y1; ++py)
        for (int px = r.x0; px < r.x1; ++px) blend(px, py, c, 1.0f);
}

void Surface::fillCircle(float cx, float cy, float radius, Color c) {
    IntRect clip = currentClip();
    int x0 = static_cast<int>(std::floor(cx - radius - 1)), x1 = static_cast<int>(std::ceil(cx + radius + 1));
    int y0 = static_cast<int>(std::floor(cy - radius - 1)), y1 = static_cast<int>(std::ceil(cy + radius + 1));
    IntRect r = IntRect{x0, y0, x1, y1}.intersect(clip);
    for (int py = r.y0; py < r.y1; ++py) {
        for (int px = r.x0; px < r.x1; ++px) {
            int hits = 0;
            for (int sy = 0; sy < 2; ++sy)
                for (int sx = 0; sx < 2; ++sx) {
                    float dx = px + 0.25f + 0.5f * sx - cx, dy = py + 0.25f + 0.5f * sy - cy;
                    if (dx * dx + dy * dy <= radius * radius) ++hits;
                }
            if (hits) blend(px, py, c, hits / 4.0f);
        }
    }
}

float Surface::drawText(float x, float y, std::string_view utf8, const TextStyle& st) {
    const float adv = glyphAdvance(st.fontSize);
    const float cellH = st.fontSize;
    IntRect clip = currentClip();
    float penX = x;
    size_t i = 0;
    while (i < utf8.size()) {
        char32_t cp = decodeUtf8(utf8, i);
        GlyphInfo g = glyphFor(cp);
        int px0 = static_cast<int>(std::floor(penX)), px1 = static_cast<int>(std::ceil(penX + adv + (st.italic ? cellH * 0.2f : 0) + (st.bold ? 1 : 0)));
        int py0 = static_cast<int>(std::floor(y)), py1 = static_cast<int>(std::ceil(y + cellH));
        IntRect r = IntRect{px0, py0, px1, py1}.intersect(clip);
        for (int py = r.y0; py < r.y1; ++py) {
            for (int px = r.x0; px < r.x1; ++px) {
                int hits = 0;
                for (int sy = 0; sy < 2; ++sy) {
                    float fy = py + 0.25f + 0.5f * sy;
                    float v = (fy - y) / cellH * 8.0f;
                    int row = static_cast<int>(std::floor(v));
                    if (row < 0 || row > 7) continue;
                    float shear = st.italic ? ((y + cellH) - fy) * 0.2f : 0.0f;
                    for (int sx = 0; sx < 2; ++sx) {
                        float fx = px + 0.25f + 0.5f * sx;
                        float u = (fx - penX - shear) / adv * 8.0f;
                        int col = static_cast<int>(std::floor(u));
                        if (u < 0 || col > 7) continue;
                        uint8_t bits = g.rows[row];
                        if (row < 2) bits |= g.mark[row];
                        bool on = (bits >> col) & 1;
                        if (!on && st.bold && col > 0) on = (bits >> (col - 1)) & 1;
                        if (on) ++hits;
                    }
                }
                if (hits) blend(px, py, st.color, hits / 4.0f);
            }
        }
        penX += adv;
    }
    float width = penX - x;
    if (st.underline) fillRect(x, y + cellH * 0.9f, width, std::max(1.0f, cellH / 14.0f), st.color);
    if (st.lineThrough) fillRect(x, y + cellH * 0.5f, width, std::max(1.0f, cellH / 14.0f), st.color);
    return width;
}

// ---- PNG ----

namespace {

uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t n) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    for (size_t i = 0; i < n; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

void put32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x >> 24)); v.push_back(static_cast<uint8_t>(x >> 16));
    v.push_back(static_cast<uint8_t>(x >> 8)); v.push_back(static_cast<uint8_t>(x));
}

void putChunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
    put32(out, static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> body(type, type + 4);
    body.insert(body.end(), data.begin(), data.end());
    out.insert(out.end(), body.begin(), body.end());
    put32(out, crc32Update(0xFFFFFFFFu, body.data(), body.size()) ^ 0xFFFFFFFFu);
}

} // namespace

std::vector<uint8_t> encodePng(const uint8_t* rgba, int width, int height, int strideBytes) {
    if (!rgba || width <= 0 || height <= 0 || strideBytes < width * 4) return {};
    std::vector<uint8_t> raw;
    raw.reserve(static_cast<size_t>(height) * (width * 4 + 1));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);  // filter: none
        raw.insert(raw.end(), rgba + static_cast<size_t>(y) * strideBytes, rgba + static_cast<size_t>(y) * strideBytes + width * 4);
    }
    std::vector<uint8_t> z = {0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (uint8_t byte : raw) { a = (a + byte) % 65521; b = (b + a) % 65521; }
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        bool last = pos + n == raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<uint8_t>(n & 0xFF)); z.push_back(static_cast<uint8_t>(n >> 8));
        z.push_back(static_cast<uint8_t>(~n & 0xFF)); z.push_back(static_cast<uint8_t>((~n >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    }
    put32(z, (b << 16) | a);

    std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::vector<uint8_t> ihdr;
    put32(ihdr, static_cast<uint32_t>(width));
    put32(ihdr, static_cast<uint32_t>(height));
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});  // 8-bit RGBA
    putChunk(out, "IHDR", ihdr);
    putChunk(out, "IDAT", z);
    putChunk(out, "IEND", {});
    return out;
}

} // namespace HB
