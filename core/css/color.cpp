// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "css/color.hpp"
#include "base/strings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace HB {

namespace {

struct Named { const char* name; uint32_t rgb; };
const Named kNames[] = {
    {"black", 0x000000}, {"silver", 0xC0C0C0}, {"gray", 0x808080}, {"grey", 0x808080}, {"white", 0xFFFFFF},
    {"maroon", 0x800000}, {"red", 0xFF0000}, {"purple", 0x800080}, {"fuchsia", 0xFF00FF}, {"magenta", 0xFF00FF},
    {"green", 0x008000}, {"lime", 0x00FF00}, {"olive", 0x808000}, {"yellow", 0xFFFF00}, {"navy", 0x000080},
    {"blue", 0x0000FF}, {"teal", 0x008080}, {"aqua", 0x00FFFF}, {"cyan", 0x00FFFF}, {"orange", 0xFFA500},
    {"pink", 0xFFC0CB}, {"brown", 0xA52A2A}, {"gold", 0xFFD700}, {"indigo", 0x4B0082}, {"violet", 0xEE82EE},
    {"lightgray", 0xD3D3D3}, {"lightgrey", 0xD3D3D3}, {"darkgray", 0xA9A9A9}, {"darkgrey", 0xA9A9A9},
    {"dimgray", 0x696969}, {"whitesmoke", 0xF5F5F5}, {"gainsboro", 0xDCDCDC}, {"lightblue", 0xADD8E6},
    {"darkblue", 0x00008B}, {"skyblue", 0x87CEEB}, {"royalblue", 0x4169E1}, {"steelblue", 0x4682B4},
    {"lightgreen", 0x90EE90}, {"darkgreen", 0x006400}, {"forestgreen", 0x228B22}, {"crimson", 0xDC143C},
    {"tomato", 0xFF6347}, {"coral", 0xFF7F50}, {"salmon", 0xFA8072}, {"khaki", 0xF0E68C}, {"beige", 0xF5F5DC},
    {"ivory", 0xFFFFF0}, {"lavender", 0xE6E6FA}, {"turquoise", 0x40E0D0}, {"tan", 0xD2B48C}, {"chocolate", 0xD2691E},
    {"darkred", 0x8B0000}, {"orangered", 0xFF4500}, {"slategray", 0x708090}, {"slategrey", 0x708090},
};

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

uint8_t clamp8(double v) { return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0, 255.0))); }

// Splits function arguments on commas, spaces and '/'.
std::vector<std::string> splitArgs(std::string_view s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',' || c == '/' || isAsciiSpace(c)) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

bool parseNumber(const std::string& s, double& value, bool& percent) {
    if (s.empty()) return false;
    char* end = nullptr;
    value = std::strtod(s.c_str(), &end);
    if (end == s.c_str()) return false;
    percent = false;
    if (*end == '%') { percent = true; ++end; }
    else if (std::string(end) == "deg") { end += 3; }
    return *end == '\0';
}

double hueToRgb(double p, double q, double t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0 / 6) return p + (q - p) * 6 * t;
    if (t < 1.0 / 2) return q;
    if (t < 2.0 / 3) return p + (q - p) * (2.0 / 3 - t) * 6;
    return p;
}

} // namespace

std::optional<Color> parseColor(std::string_view text) {
    std::string t = toLowerAscii(trim(text));
    if (t.empty()) return std::nullopt;
    if (t == "transparent") return Color::transparent();
    if (t[0] == '#') {
        std::string h = t.substr(1);
        for (char c : h) if (hexVal(c) < 0) return std::nullopt;
        Color c;
        if (h.size() == 3 || h.size() == 4) {
            c.r = static_cast<uint8_t>(hexVal(h[0]) * 17);
            c.g = static_cast<uint8_t>(hexVal(h[1]) * 17);
            c.b = static_cast<uint8_t>(hexVal(h[2]) * 17);
            c.a = h.size() == 4 ? static_cast<uint8_t>(hexVal(h[3]) * 17) : 255;
            return c;
        }
        if (h.size() == 6 || h.size() == 8) {
            c.r = static_cast<uint8_t>(hexVal(h[0]) * 16 + hexVal(h[1]));
            c.g = static_cast<uint8_t>(hexVal(h[2]) * 16 + hexVal(h[3]));
            c.b = static_cast<uint8_t>(hexVal(h[4]) * 16 + hexVal(h[5]));
            c.a = h.size() == 8 ? static_cast<uint8_t>(hexVal(h[6]) * 16 + hexVal(h[7])) : 255;
            return c;
        }
        return std::nullopt;
    }
    size_t paren = t.find('(');
    if (paren != std::string::npos && t.back() == ')') {
        std::string fn = std::string(trim(std::string_view(t).substr(0, paren)));
        auto args = splitArgs(std::string_view(t).substr(paren + 1, t.size() - paren - 2));
        if ((fn == "rgb" || fn == "rgba") && (args.size() == 3 || args.size() == 4)) {
            double v[4] = {0, 0, 0, 1};
            for (size_t i = 0; i < args.size(); ++i) {
                bool pct = false;
                if (!parseNumber(args[i], v[i], pct)) return std::nullopt;
                if (i < 3) { if (pct) v[i] = v[i] * 255.0 / 100.0; }
                else if (pct) v[i] /= 100.0;
            }
            return Color{clamp8(v[0]), clamp8(v[1]), clamp8(v[2]), clamp8(v[3] * 255.0)};
        }
        if ((fn == "hsl" || fn == "hsla") && (args.size() == 3 || args.size() == 4)) {
            double h, s, l, a = 1;
            bool p1, p2, p3, p4 = false;
            if (!parseNumber(args[0], h, p1) || !parseNumber(args[1], s, p2) || !parseNumber(args[2], l, p3)) return std::nullopt;
            if (args.size() == 4 && !parseNumber(args[3], a, p4)) return std::nullopt;
            if (p4) a /= 100.0;
            h = std::fmod(std::fmod(h, 360.0) + 360.0, 360.0) / 360.0;
            s = std::clamp(s / 100.0, 0.0, 1.0);
            l = std::clamp(l / 100.0, 0.0, 1.0);
            double r, g, b;
            if (s == 0) r = g = b = l;
            else {
                double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
                double p = 2 * l - q;
                r = hueToRgb(p, q, h + 1.0 / 3);
                g = hueToRgb(p, q, h);
                b = hueToRgb(p, q, h - 1.0 / 3);
            }
            return Color{clamp8(r * 255), clamp8(g * 255), clamp8(b * 255), clamp8(a * 255)};
        }
        return std::nullopt;
    }
    for (const auto& n : kNames) {
        if (t == n.name) return Color{static_cast<uint8_t>(n.rgb >> 16), static_cast<uint8_t>((n.rgb >> 8) & 0xFF), static_cast<uint8_t>(n.rgb & 0xFF), 255};
    }
    return std::nullopt;
}

} // namespace HB
