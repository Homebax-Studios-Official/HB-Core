// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace HB {

struct OutlinePoint { float x, y; bool onCurve; };
using Contour = std::vector<OutlinePoint>;

struct GlyphBitmap {
    int left = 0;   // pixel offset of the bitmap's left edge from the pen x
    int top = 0;    // pixel offset of the bitmap's top edge from the baseline (negative = above)
    int width = 0;
    int height = 0;
    std::vector<uint8_t> alpha;
};

// Reader for TrueType (glyf) fonts: .ttf and the first font of a .ttc. OpenType/CFF ("OTTO") is not supported.
// Uses cmap formats 4 and 12, hmtx, loca/glyf with composite glyphs. No hinting, kerning or shaping.
class TrueTypeFont {
public:
    static std::unique_ptr<TrueTypeFont> load(std::vector<uint8_t> data);

    int unitsPerEm() const { return unitsPerEm_; }
    float ascent() const { return ascent_; }    // font units
    float descent() const { return descent_; }  // font units, positive
    uint16_t glyphIndex(char32_t cp) const;     // 0 = missing
    float advance(uint16_t glyph) const;        // font units
    bool outline(uint16_t glyph, std::vector<Contour>& out) const;

    // Rasterizes with 4x vertical supersampling and exact horizontal coverage.
    // shear: x += shear * y (synthetic italic); embolden: widen strokes by ~1 px at large sizes.
    GlyphBitmap rasterize(uint16_t glyph, float pixelSize, float shear, bool embolden) const;

private:
    TrueTypeFont() = default;
    bool parse();
    bool loadGlyph(uint16_t glyph, std::vector<Contour>& out, int depth) const;

    std::vector<uint8_t> d_;
    uint32_t glyf_ = 0, loca_ = 0, cmap_ = 0, hmtx_ = 0;
    uint32_t glyfLen_ = 0;
    int unitsPerEm_ = 1000, numGlyphs_ = 0, numHMetrics_ = 0;
    bool longLoca_ = false;
    float ascent_ = 800, descent_ = 200;
    uint32_t cmapSub_ = 0;
    int cmapFormat_ = 0;
};

} // namespace HB
