// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstdint>
#include <optional>
#include <string_view>

namespace HB {

struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    static Color transparent() { return {0, 0, 0, 0}; }
};

// Parses CSS colors: named colors (common subset), #rgb, #rgba, #rrggbb, #rrggbbaa,
// rgb()/rgba(), hsl()/hsla(), "transparent". "currentcolor" is not handled here.
std::optional<Color> parseColor(std::string_view text);

} // namespace HB
