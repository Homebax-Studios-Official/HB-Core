// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "graphics/fonts.hpp"
#include "graphics/bitmap_font.hpp"
#include "base/strings.hpp"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace HB {

namespace {

std::vector<uint8_t> readAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    std::streamoff n = f.tellg();
    if (n <= 0 || n > 64 * 1024 * 1024) return {};
    f.seekg(0);
    std::vector<uint8_t> data(static_cast<size_t>(n));
    f.read(reinterpret_cast<char*>(data.data()), n);
    return f ? data : std::vector<uint8_t>();
}

struct FamilyFiles { const char* regular; const char* bold; const char* italic; const char* boldItalic; };

struct Candidate { FontFamily family; FamilyFiles files; };

const Candidate kCandidates[] = {
    // Sans
    {FontFamily::Sans, {"segoeui.ttf", "segoeuib.ttf", "segoeuii.ttf", "segoeuiz.ttf"}},
    {FontFamily::Sans, {"arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf"}},
    {FontFamily::Sans, {"LiberationSans-Regular.ttf", "LiberationSans-Bold.ttf", "LiberationSans-Italic.ttf", "LiberationSans-BoldItalic.ttf"}},
    {FontFamily::Sans, {"DejaVuSans.ttf", "DejaVuSans-Bold.ttf", "DejaVuSans-Oblique.ttf", "DejaVuSans-BoldOblique.ttf"}},
    {FontFamily::Sans, {"NotoSans-Regular.ttf", "NotoSans-Bold.ttf", "NotoSans-Italic.ttf", "NotoSans-BoldItalic.ttf"}},
    {FontFamily::Sans, {"Roboto-Regular.ttf", "Roboto-Bold.ttf", "Roboto-Italic.ttf", "Roboto-BoldItalic.ttf"}},
    // Serif
    {FontFamily::Serif, {"times.ttf", "timesbd.ttf", "timesi.ttf", "timesbi.ttf"}},
    {FontFamily::Serif, {"LiberationSerif-Regular.ttf", "LiberationSerif-Bold.ttf", "LiberationSerif-Italic.ttf", "LiberationSerif-BoldItalic.ttf"}},
    {FontFamily::Serif, {"DejaVuSerif.ttf", "DejaVuSerif-Bold.ttf", "DejaVuSerif-Italic.ttf", "DejaVuSerif-BoldItalic.ttf"}},
    {FontFamily::Serif, {"NotoSerif-Regular.ttf", "NotoSerif-Bold.ttf", "NotoSerif-Italic.ttf", "NotoSerif-BoldItalic.ttf"}},
    // Mono
    {FontFamily::Mono, {"consola.ttf", "consolab.ttf", "consolai.ttf", "consolaz.ttf"}},
    {FontFamily::Mono, {"cour.ttf", "courbd.ttf", "couri.ttf", "courbi.ttf"}},
    {FontFamily::Mono, {"LiberationMono-Regular.ttf", "LiberationMono-Bold.ttf", "LiberationMono-Italic.ttf", "LiberationMono-BoldItalic.ttf"}},
    {FontFamily::Mono, {"DejaVuSansMono.ttf", "DejaVuSansMono-Bold.ttf", "DejaVuSansMono-Oblique.ttf", "DejaVuSansMono-BoldOblique.ttf"}},
    {FontFamily::Mono, {"NotoSansMono-Regular.ttf", "NotoSansMono-Bold.ttf", nullptr, nullptr}},
};

std::vector<std::string> fontDirs(const std::vector<std::string>& extra) {
    std::vector<std::string> dirs;
    for (const auto& d : extra) dirs.push_back(d.empty() || d.back() == '/' || d.back() == '\\' ? d : d + "/");
    if (const char* extra = std::getenv("HBCORE_FONT_DIR")) dirs.push_back(std::string(extra) + "/");
    if (const char* win = std::getenv("WINDIR")) dirs.push_back(std::string(win) + "/Fonts/");
    dirs.push_back("C:/Windows/Fonts/");
    for (const char* d : {"/usr/share/fonts/truetype/liberation/", "/usr/share/fonts/truetype/liberation2/", "/usr/share/fonts/truetype/dejavu/",
                          "/usr/share/fonts/truetype/noto/", "/usr/share/fonts/liberation/", "/usr/share/fonts/dejavu/",
                          "/usr/share/fonts/TTF/", "/usr/share/fonts/noto/", "/usr/share/fonts/google-noto/", "/system/fonts/",
                          "/Library/Fonts/", "/System/Library/Fonts/Supplemental/"})
        dirs.push_back(d);
    return dirs;
}

}  // namespace

FontProvider& FontProvider::instance() {
    static FontProvider p;
    return p;
}

void FontProvider::reset() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    cache_.clear();
    for (auto& fam : slots_) for (auto& s : fam) { s.font.reset(); s.path.clear(); }
}

bool FontProvider::loadFile(FontFamily family, bool bold, bool italic, const std::string& path) {
    auto font = TrueTypeFont::load(readAll(path));
    if (!font) return false;
    std::lock_guard<std::mutex> lock(cacheMutex_);
    Slot& s = slots_[static_cast<int>(family)][(bold ? 1 : 0) + (italic ? 2 : 0)];
    cache_.clear();
    s.font = std::move(font);
    s.path = path;
    return true;
}

int FontProvider::discoverSystemFonts() {
    int found = 0;
    auto dirs = fontDirs(extraDirs_);
    for (int fam = 0; fam < 3; ++fam) {
        if (slots_[fam][0].font) { ++found; continue; }
        for (const auto& c : kCandidates) {
            if (static_cast<int>(c.family) != fam) continue;
            for (const auto& dir : dirs) {
                if (!loadFile(c.family, false, false, dir + c.files.regular)) continue;
                const char* names[3] = {c.files.bold, c.files.italic, c.files.boldItalic};
                for (int i = 0; i < 3; ++i)
                    if (names[i]) loadFile(c.family, (i == 0 || i == 2), (i == 1 || i == 2), dir + names[i]);
                ++found;
                goto nextFamily;
            }
        }
    nextFamily:;
    }
    return found;
}

void FontProvider::addSearchDirectory(const std::string& dir) {
    if (!dir.empty()) extraDirs_.push_back(dir);
}

bool FontProvider::available() const {
    for (const auto& fam : slots_) if (fam[0].font) return true;
    return false;
}

FaceRef FontProvider::select(FontFamily family, bool bold, bool italic) const {
    for (int attempt = 0; attempt < 2; ++attempt) {
        int fam = attempt == 0 ? static_cast<int>(family) : static_cast<int>(FontFamily::Sans);
        const auto& f = slots_[fam];
        int want = (bold ? 1 : 0) + (italic ? 2 : 0);
        if (f[want].font) return {f[want].font.get(), false, false};
        if (!f[0].font) continue;
        // Fall back to a related face and synthesize the missing style.
        if (bold && italic) {
            if (f[2].font) return {f[2].font.get(), true, false};
            if (f[1].font) return {f[1].font.get(), false, true};
            return {f[0].font.get(), true, true};
        }
        return {f[0].font.get(), bold, italic};
    }
    return {};
}

std::shared_ptr<const GlyphBitmap> FontProvider::bitmap(const FaceRef& face, char32_t cp, float size) {
    if (!face.font) return nullptr;
    uint16_t g = face.font->glyphIndex(cp);
    int sizeKey = static_cast<int>(std::lround(size * 8));
    int flags = (face.fakeBold ? 1 : 0) | (face.fakeItalic ? 2 : 0);
    auto key = std::make_tuple(face.font, g, sizeKey, flags);
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
    }
    auto bm = std::make_shared<GlyphBitmap>(face.font->rasterize(g, size, face.fakeItalic ? 0.2f : 0.0f, face.fakeBold));
    std::lock_guard<std::mutex> lock(cacheMutex_);
    if (cache_.size() > 8192) cache_.clear();
    cache_[key] = bm;
    return bm;
}

float FontProvider::advance(const FaceRef& face, char32_t cp, float size) const {
    if (!face.font) return glyphAdvance(size);
    return face.font->advance(face.font->glyphIndex(cp)) * size / static_cast<float>(face.font->unitsPerEm());
}

float FontProvider::ascentPx(const FaceRef& face, float size) const {
    return face.font ? face.font->ascent() * size / static_cast<float>(face.font->unitsPerEm()) : size * 0.8f;
}

float FontProvider::descentPx(const FaceRef& face, float size) const {
    return face.font ? face.font->descent() * size / static_cast<float>(face.font->unitsPerEm()) : size * 0.2f;
}

std::string FontProvider::describe() const {
    static const char* names[3] = {"sans", "serif", "mono"};
    std::string out;
    for (int f = 0; f < 3; ++f) {
        if (!slots_[f][0].font) continue;
        if (!out.empty()) out += "; ";
        out += std::string(names[f]) + "=" + slots_[f][0].path;
    }
    return out.empty() ? "built-in bitmap font" : out;
}

float styledAdvance(char32_t cp, float fontSize, FontFamily family, bool bold, bool italic) {
    FontProvider& p = FontProvider::instance();
    return p.advance(p.select(family, bold, italic), cp, fontSize);
}

float measureStyledText(std::string_view utf8, float fontSize, FontFamily family, bool bold, bool italic) {
    FontProvider& p = FontProvider::instance();
    FaceRef face = p.select(family, bold, italic);
    float w = 0;
    size_t i = 0;
    while (i < utf8.size()) w += p.advance(face, decodeUtf8(utf8, i), fontSize);
    return w;
}

} // namespace HB
