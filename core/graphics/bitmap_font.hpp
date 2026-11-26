// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstdint>
#include <string_view>

namespace HB {

// Built-in 8x8 monospaced bitmap font (ASCII glyph data based on the public-domain "font8x8"
// by Daniel Hepper; see THIRD_PARTY_NOTICES.md). Latin diacritics used in Czech, Slovak,
// German and French are drawn as a base glyph plus an accent mark. Unknown code points are
// drawn as '?'. This is a placeholder until FreeType/HarfBuzz text rendering exists.
struct GlyphInfo {
    const uint8_t* rows;  // 8 rows; bit 0 = leftmost column
    uint8_t mark[2];      // accent bitmap for rows 0 and 1 (0 = none)
};

GlyphInfo glyphFor(char32_t cp);

inline float glyphAdvance(float fontSize) { return fontSize * 0.6f; }
float measureText(std::string_view utf8, float fontSize);

} // namespace HB
