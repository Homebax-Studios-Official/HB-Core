#include "test_support.h"
#include "base/strings.hpp"
#include "graphics/fonts.hpp"
#include "graphics/image.hpp"
#include "graphics/surface.hpp"
#include "graphics/truetype.hpp"
#include "test_png_data.h"
#include <fstream>
#include <iterator>
using namespace HB;

TEST(Png, DecodesAllGeneratedVariantsExactly) {
    for (const auto& c : pngCases()) {
        auto img = decodePng(c.png.data(), c.png.size());
        if (!img) { EXPECT_TRUE(false); std::cerr << "decode failed: " << c.name << "\n"; continue; }
        EXPECT_EQ(img->width, c.w);
        EXPECT_EQ(img->height, c.h);
        bool same = img->rgba == c.rgba;
        if (!same) std::cerr << "pixel mismatch in " << c.name << "\n";
        EXPECT_TRUE(same);
    }
}

TEST(Png, RoundTripWithOwnEncoder) {
    Surface s(5, 4);
    s.clear({10, 20, 30, 255});
    s.fillRect(1, 1, 2, 2, {200, 100, 50, 255});
    auto png = encodePng(s.data(), 5, 4, s.stride());
    auto img = decodePng(png.data(), png.size());
    ASSERT_TRUE(img.has_value());
    EXPECT_EQ(img->width, 5);
    EXPECT_EQ(int(img->rgba[(1 * 5 + 1) * 4]), 200);
    EXPECT_EQ(int(img->rgba[0]), 10);
}

TEST(Png, MalformedInputIsRejectedNotCrashed) {
    auto good = pngCases()[0].png;
    EXPECT_FALSE(decodePng(nullptr, 0).has_value());
    EXPECT_FALSE(decodePng(good.data(), 10).has_value());
    for (size_t cut = 8; cut < good.size(); cut += 7) decodePng(good.data(), cut);  // truncations
    // bit flips anywhere must never crash (result may be nullopt or garbage pixels)
    for (size_t i = 0; i < good.size(); ++i) {
        auto bad = good;
        bad[i] ^= 0xFF;
        decodePng(bad.data(), bad.size());
    }
    // huge dimensions are refused before allocating
    auto huge = good;
    huge[16] = 0x7F;  // width = 0x7F00000D
    EXPECT_FALSE(decodePng(huge.data(), huge.size()).has_value());
}

TEST(Inflate, StoredBlockAndErrors) {
    std::vector<uint8_t> z = {0x78, 0x01, 0x01, 0x03, 0x00, 0xFC, 0xFF, 'a', 'b', 'c', 0, 0, 0, 0};
    std::vector<uint8_t> out;
    EXPECT_TRUE(zlibInflate(z.data(), z.size(), out, 100));
    EXPECT_EQ(std::string(out.begin(), out.end()), std::string("abc"));
    EXPECT_FALSE(zlibInflate(z.data(), z.size(), out, 2));  // output cap
    z[2] = 0x07;  // reserved block type 3
    EXPECT_FALSE(zlibInflate(z.data(), z.size(), out, 100));
}

TEST(Base64, DecodeAndInvalid) {
    bool ok = false;
    EXPECT_EQ(base64Decode("SGVsbG8gSEIgQ29yZQ==", &ok), std::string("Hello HB Core"));
    EXPECT_TRUE(ok);
    EXPECT_EQ(base64Decode("SGVs bG8", &ok), std::string("Hello"));
    EXPECT_TRUE(ok);
    base64Decode("!!!", &ok);
    EXPECT_FALSE(ok);
}

TEST(Surface, DrawImageScalesAndBlends) {
    auto img = decodePng(pngCases()[0].png.data(), pngCases()[0].png.size());
    ASSERT_TRUE(img.has_value());
    Image red;
    red.width = 2; red.height = 2;
    red.rgba = {255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255};
    Surface s(20, 20);
    s.clear({255, 255, 255, 255});
    s.drawImage(5, 5, 10, 10, red);
    EXPECT_TRUE(s.pixel(10, 10) == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(s.pixel(2, 2) == (Color{255, 255, 255, 255}));
    s.pushClip(0, 0, 8, 8);
    s.drawImage(0, 0, 20, 20, red);
    s.popClip();
    EXPECT_TRUE(s.pixel(12, 12) == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(s.pixel(18, 18) == (Color{255, 255, 255, 255}));
}

namespace {
const char* findFont() {
    static const char* candidates[] = {
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf",
    };
    for (const char* c : candidates) { std::ifstream f(c, std::ios::binary); if (f) return c; }
    return nullptr;
}
std::vector<uint8_t> readFile(const char* p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
}

TEST(TrueType, LoadsMetricsGlyphsAndRasterizes) {
    const char* path = findFont();
    if (!path) { std::cerr << "(no system TrueType font found; test skipped)\n"; return; }
    auto font = TrueTypeFont::load(readFile(path));
    ASSERT_TRUE(font != nullptr);
    uint16_t a = font->glyphIndex('A'), iDot = font->glyphIndex('i');
    EXPECT_NE(int(a), 0);
    EXPECT_NE(int(a), int(iDot));
    EXPECT_GT(font->advance(a), 0.0f);
    EXPECT_GT(font->advance(font->glyphIndex('W')), font->advance(iDot));  // proportional
    GlyphBitmap bm = font->rasterize(a, 32, 0, false);
    ASSERT_GT(bm.width, 5);
    int ink = 0;
    for (uint8_t v : bm.alpha) ink += v > 128;
    EXPECT_GT(ink, 100);
    // Czech letter (composite or precomposed glyph) must exist and have ink
    uint16_t z = font->glyphIndex(0x17E);
    EXPECT_NE(int(z), 0);
    GlyphBitmap zb = font->rasterize(z, 32, 0, false);
    EXPECT_GT(zb.width, 3);
    // bold smear never shrinks coverage
    GlyphBitmap fat = font->rasterize(a, 32, 0, true);
    long s1 = 0, s2 = 0;
    for (uint8_t v : bm.alpha) s1 += v;
    for (uint8_t v : fat.alpha) s2 += v;
    EXPECT_GT(s2, s1);
}

TEST(TrueType, RejectsGarbageAndTruncatedFonts) {
    EXPECT_TRUE(TrueTypeFont::load({}) == nullptr);
    EXPECT_TRUE(TrueTypeFont::load(std::vector<uint8_t>(200, 0xAB)) == nullptr);
    const char* path = findFont();
    if (!path) return;
    auto data = readFile(path);
    for (size_t cut : {size_t(100), size_t(2000), data.size() / 2}) {
        std::vector<uint8_t> part(data.begin(), data.begin() + cut);
        auto f = TrueTypeFont::load(part);
        if (f) f->rasterize(f->glyphIndex('A'), 20, 0, false);  // must not crash
    }
}

TEST(FontProvider, TextUsesOutlineFontWhenLoadedAndFallsBackOtherwise) {
    FontProvider& fp = FontProvider::instance();
    fp.reset();
    EXPECT_FALSE(fp.available());
    EXPECT_NEAR(measureStyledText("abcd", 10, FontFamily::Sans, false, false), 24, 0.001);  // bitmap fallback: 0.6 em
    const char* path = findFont();
    if (!path) return;
    EXPECT_FALSE(fp.loadFile(FontFamily::Sans, false, false, "/no/such/font.ttf"));
    ASSERT_TRUE(fp.loadFile(FontFamily::Sans, false, false, path));
    EXPECT_TRUE(fp.available());
    float wI = measureStyledText("iiii", 20, FontFamily::Sans, false, false);
    float wW = measureStyledText("WWWW", 20, FontFamily::Sans, false, false);
    EXPECT_GT(wW, wI * 1.5f);
    FaceRef bold = fp.select(FontFamily::Sans, true, false);
    EXPECT_TRUE(bold.fakeBold);  // only the regular file was loaded -> synthesized
    Surface s(120, 40);
    s.clear({255, 255, 255, 255});
    TextStyle ts; ts.fontSize = 24;
    s.drawText(4, 4, "Hello \xC5\xBEl", ts);
    int ink = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 120; ++x) if (s.pixel(x, y).r < 128) ++ink;
    EXPECT_GT(ink, 150);
    fp.reset();
}
