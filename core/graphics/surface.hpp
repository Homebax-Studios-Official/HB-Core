// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "css/color.hpp"

namespace HB {

struct IntRect {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;  // [x0,x1) x [y0,y1)
    bool empty() const { return x1 <= x0 || y1 <= y0; }
    IntRect intersect(const IntRect& o) const;
};

struct TextStyle {
    float fontSize = 16;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool lineThrough = false;
    Color color{0, 0, 0, 255};
};

// CPU software rendering surface: RGBA8, straight alpha, source-over blending.
class Surface {
public:
    Surface(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }
    int stride() const { return width_ * 4; }
    const uint8_t* data() const { return pixels_.data(); }
    uint8_t* data() { return pixels_.data(); }
    void resize(int width, int height);

    void clear(Color c);
    // Coordinates are in surface pixels. Edges are snapped to whole pixels; non-empty rects are at least 1px.
    void fillRect(float x, float y, float w, float h, Color c);
    void fillCircle(float cx, float cy, float radius, Color c);
    // y is the top of the text cell; the cell height equals fontSize. Returns the advance width.
    float drawText(float x, float y, std::string_view utf8, const TextStyle& style);

    void pushClip(float x, float y, float w, float h);
    void popClip();
    Color pixel(int x, int y) const;

private:
    void blend(int x, int y, Color c, float coverage);
    IntRect currentClip() const;

    int width_, height_;
    std::vector<uint8_t> pixels_;
    std::vector<IntRect> clips_;
};

// Encodes RGBA8 as a PNG using stored (uncompressed) deflate blocks. Returns an empty vector on bad input.
std::vector<uint8_t> encodePng(const uint8_t* rgba, int width, int height, int strideBytes);

} // namespace HB
