// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "base/strings.hpp"

namespace HB {

bool isAsciiSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
bool isAsciiAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool isAsciiDigit(char c) { return c >= '0' && c <= '9'; }
bool isAsciiAlnum(char c) { return isAsciiAlpha(c) || isAsciiDigit(c); }

std::string toLowerAscii(std::string_view s) {
    std::string r(s);
    for (char& c : r) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return r;
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = static_cast<char>(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = static_cast<char>(y - 'A' + 'a');
        if (x != y) return false;
    }
    return true;
}

std::string_view trim(std::string_view s) {
    size_t b = 0, e = s.size();
    while (b < e && isAsciiSpace(s[b])) ++b;
    while (e > b && isAsciiSpace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

bool startsWith(std::string_view s, std::string_view p) { return s.size() >= p.size() && s.compare(0, p.size(), p) == 0; }
bool endsWith(std::string_view s, std::string_view p) { return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0; }

std::vector<std::string> splitWhitespace(std::string_view s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && isAsciiSpace(s[i])) ++i;
        size_t b = i;
        while (i < s.size() && !isAsciiSpace(s[i])) ++i;
        if (i > b) out.emplace_back(s.substr(b, i - b));
    }
    return out;
}

char32_t decodeUtf8(std::string_view s, size_t& pos) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t n = s.size();
    unsigned char c = p[pos];
    auto cont = [&](size_t k) { return pos + k < n && (p[pos + k] & 0xC0) == 0x80; };
    if (c < 0x80) { ++pos; return c; }
    if (c >= 0xC2 && c <= 0xDF && cont(1)) {
        char32_t cp = ((c & 0x1F) << 6) | (p[pos + 1] & 0x3F);
        pos += 2; return cp;
    }
    if (c >= 0xE0 && c <= 0xEF && cont(1) && cont(2)) {
        char32_t cp = ((c & 0x0F) << 12) | ((p[pos + 1] & 0x3F) << 6) | (p[pos + 2] & 0x3F);
        if (cp >= 0x800 && !(cp >= 0xD800 && cp <= 0xDFFF)) { pos += 3; return cp; }
    }
    if (c >= 0xF0 && c <= 0xF4 && cont(1) && cont(2) && cont(3)) {
        char32_t cp = ((c & 0x07) << 18) | ((p[pos + 1] & 0x3F) << 12) | ((p[pos + 2] & 0x3F) << 6) | (p[pos + 3] & 0x3F);
        if (cp >= 0x10000 && cp <= 0x10FFFF) { pos += 4; return cp; }
    }
    ++pos;
    return 0xFFFD;
}

std::u32string utf8ToUtf32(std::string_view s) {
    std::u32string out;
    size_t i = 0;
    while (i < s.size()) out.push_back(decodeUtf8(s, i));
    return out;
}

void appendUtf8(std::string& out, char32_t cp) {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
    if (cp < 0x80) out.push_back(static_cast<char>(cp));
    else if (cp < 0x800) { out.push_back(static_cast<char>(0xC0 | (cp >> 6))); out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

} // namespace HB
