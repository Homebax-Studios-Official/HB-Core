#include "test_support.h"
#include "graphics/bitmap_font.hpp"
#include "graphics/surface.hpp"
using namespace HB;

TEST(Font, GlyphTableSanity) {
    EXPECT_EQ(int(glyphFor('~').rows[0]), 0x6E);
    EXPECT_EQ(int(glyphFor('A').rows[0]), 0x0C);
    EXPECT_EQ(int(glyphFor('z').rows[6]), 0x3F);
    EXPECT_EQ(int(glyphFor(' ').rows[3]), 0);
    // diacritics fold to the base glyph and add a mark
    auto s = glyphFor(0x161);
    EXPECT_EQ(int(s.rows[0]), int(glyphFor('s').rows[0]));
    EXPECT_NE(int(s.mark[0]), 0);
    // unknown code point renders as '?'
    EXPECT_EQ(int(glyphFor(0x4E2D).rows[0]), int(glyphFor('?').rows[0]));
    EXPECT_NEAR(measureText("abc", 10), 18, 0.001);
    EXPECT_NEAR(measureText("\xC5\xA1\xC4\x8D", 10), 12, 0.001);
}

TEST(Surface, FillRectClipAndBlend) {
    Surface s(20, 20);
    s.clear({255, 255, 255, 255});
    s.fillRect(2, 2, 5, 5, {255, 0, 0, 255});
    EXPECT_TRUE(s.pixel(2, 2) == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(s.pixel(7, 7) == (Color{255, 255, 255, 255}));
    s.pushClip(10, 10, 3, 3);
    s.fillRect(0, 0, 20, 20, {0, 0, 255, 255});
    s.popClip();
    EXPECT_TRUE(s.pixel(11, 11) == (Color{0, 0, 255, 255}));
    EXPECT_TRUE(s.pixel(14, 14) == (Color{255, 255, 255, 255}));
    s.fillRect(15, 0, 1, 1, {0, 0, 0, 128});
    EXPECT_NEAR(s.pixel(15, 0).r, 127, 2);
    s.fillRect(0, 18, 20, 0.2f, {0, 0, 0, 255});  // thin rects stay visible
    EXPECT_TRUE(s.pixel(3, 18) == (Color{0, 0, 0, 255}));
}

TEST(Surface, DrawTextProducesInkInsideCellOnly) {
    Surface s(100, 30);
    s.clear({255, 255, 255, 255});
    TextStyle ts;
    ts.fontSize = 16;
    float w = s.drawText(10, 5, "HB", ts);
    EXPECT_NEAR(w, 19.2, 0.01);
    int ink = 0, outside = 0;
    for (int y = 0; y < 30; ++y)
        for (int x = 0; x < 100; ++x) {
            if (s.pixel(x, y).r < 200) { ++ink; if (x < 10 || x > 30 || y < 5 || y > 21) ++outside; }
        }
    EXPECT_GT(ink, 20);
    EXPECT_EQ(outside, 0);
}

TEST(Png, StructureIsValid) {
    Surface s(3, 2);
    s.clear({10, 20, 30, 255});
    auto png = encodePng(s.data(), 3, 2, s.stride());
    ASSERT_GT(png.size(), size_t(60));
    EXPECT_EQ(int(png[0]), 0x89);
    EXPECT_EQ(int(png[1]), int('P'));
    EXPECT_EQ(int(png[18]), 0);  // width = 3 (bytes 16..19)
    EXPECT_EQ(int(png[19]), 3);
    EXPECT_EQ(int(png[png.size() - 12]), 0);  // IEND: 4-byte length (0), type, CRC
    EXPECT_EQ(int(png[png.size() - 8]), int('I'));
    EXPECT_EQ(int(png[png.size() - 5]), int('D'));
    EXPECT_TRUE(encodePng(nullptr, 3, 2, 12).empty());
}
