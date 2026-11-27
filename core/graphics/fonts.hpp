// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include "css/font_family.hpp"
#include "graphics/truetype.hpp"

namespace HB {

struct FaceRef {
    const TrueTypeFont* font = nullptr;  // nullptr -> use the built-in bitmap font
    bool fakeBold = false;
    bool fakeItalic = false;
};

// Process-wide font registry. Outline fonts are loaded from files (system fonts or paths given in
// CoreConfig). If no font is available, text falls back to the built-in 8x8 bitmap font.
class FontProvider {
public:
    static FontProvider& instance();

    void reset();
    // slot: bold/italic combination. Returns false if the file is missing or not a supported TrueType font.
    bool loadFile(FontFamily family, bool bold, bool italic, const std::string& path);
    // Looks for well-known system fonts (Windows, Linux, Android). Returns the number of families found.
    int discoverSystemFonts();
    // Extra directory searched before the system locations by discoverSystemFonts().
    void addSearchDirectory(const std::string& dir);
    bool available() const;

    FaceRef select(FontFamily family, bool bold, bool italic) const;
    std::shared_ptr<const GlyphBitmap> bitmap(const FaceRef& face, char32_t cp, float size);
    // Metrics in pixels for the given size.
    float advance(const FaceRef& face, char32_t cp, float size) const;
    float ascentPx(const FaceRef& face, float size) const;
    float descentPx(const FaceRef& face, float size) const;
    std::string describe() const;  // e.g. "sans: LiberationSans-Regular.ttf ..." for logging

private:
    struct Slot { std::unique_ptr<TrueTypeFont> font; std::string path; };
    std::vector<std::string> extraDirs_;
    Slot slots_[3][4];  // [family][bold + 2*italic]
    mutable std::mutex cacheMutex_;
    std::map<std::tuple<const TrueTypeFont*, uint16_t, int, int>, std::shared_ptr<GlyphBitmap>> cache_;
};

// Text metrics that honour the active fonts (outline font if available, bitmap font otherwise).
float measureStyledText(std::string_view utf8, float fontSize, FontFamily family, bool bold, bool italic);
float styledAdvance(char32_t cp, float fontSize, FontFamily family, bool bold, bool italic);

} // namespace HB
