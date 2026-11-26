#include "test_support.h"
#include "engine/page.hpp"
using namespace HB;

static const char* kHello = "<!DOCTYPE html><html><head><title>HB Core Test</title></head><body><h1>Hello HB Core</h1><p>Independent browser engine.</p></body></html>";

static int countInk(const Surface& s, int x0, int y0, int x1, int y1) {
    int n = 0;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) { Color c = s.pixel(x, y); if (c.r < 128 && c.g < 128 && c.b < 128) ++n; }
    return n;
}

// Graphical regression test: the first-milestone page must actually be rendered as pixels.
TEST(Page, HelloPageIsRendered) {
    Page p;
    p.setViewport(400, 200, 1.0f);
    p.setSource(kHello, {});
    EXPECT_EQ(p.title(), std::string("HB Core Test"));
    const Surface& s = p.render();
    EXPECT_EQ(s.width(), 400);
    EXPECT_TRUE(s.pixel(399, 199) == (Color{255, 255, 255, 255}));
    // h1: top 21.44px, 38.4px high -> rows 21..60, text starts at x=8; paragraph below
    EXPECT_GT(countInk(s, 8, 21, 8 + 13 * 19, 60), 300);
    EXPECT_GT(countInk(s, 8, 85, 8 + 28 * 10, 105), 80);
    EXPECT_EQ(countInk(s, 300, 10, 400, 200), 0);  // nothing right of the text
    EXPECT_EQ(countInk(s, 0, 0, 8, 200), 0);       // body margin stays blank
}

TEST(Page, BackgroundsBordersAndLinksPaint) {
    Page p;
    p.setViewport(200, 100, 1.0f);
    p.setSource("<body style='margin:0'><div style='background:#ff0000;border:4px solid #0000ff;width:50px;height:20px'></div></body>", {});
    const Surface& s = p.render();
    EXPECT_TRUE(s.pixel(1, 1) == (Color{0, 0, 255, 255}));
    EXPECT_TRUE(s.pixel(10, 10) == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(s.pixel(100, 10) == (Color{255, 255, 255, 255}));
}

TEST(Page, ExternalStylesheetViaLoader) {
    Page p;
    p.setViewport(100, 50, 1.0f);
    p.setSource("<head><link rel=stylesheet href='a.css'></head><body style='margin:0'><div id=d>x</div></body>",
                [](const std::string& href) -> std::optional<std::string> {
                    if (href == "a.css") return std::string("body{background:#00ff00}");
                    return std::nullopt;
                });
    EXPECT_TRUE(p.render().pixel(90, 40) == (Color{0, 255, 0, 255}));
}

TEST(Page, ScrollingMovesContentAndClamps) {
    std::string html = "<body style='margin:0'>";
    for (int i = 0; i < 40; ++i) html += "<div style='height:20px;background:#" + std::string(i % 2 ? "000000" : "ff0000") + "'></div>";
    Page p;
    p.setViewport(50, 100, 1.0f);
    p.setSource(html, {});
    EXPECT_NEAR(p.documentHeight(), 800, 0.01);
    EXPECT_TRUE(p.render().pixel(5, 5) == (Color{255, 0, 0, 255}));
    p.setScrollY(20);
    EXPECT_TRUE(p.render().pixel(5, 5) == (Color{0, 0, 0, 255}));
    p.setScrollY(100000);
    p.render();
    EXPECT_NEAR(p.scrollY(), 700, 0.01);
    p.setScrollY(-5);
    EXPECT_NEAR(p.scrollY(), 0, 0.01);
}

TEST(Page, DevicePixelRatioScalesSurface) {
    Page p;
    p.setViewport(100, 50, 2.0f);
    p.setSource("<body style='margin:0;background:#00f'>x</body>", {});
    const Surface& s = p.render();
    EXPECT_EQ(s.width(), 200);
    EXPECT_EQ(s.height(), 100);
}

TEST(Page, OverflowHiddenClips) {
    Page p;
    p.setViewport(100, 100, 1.0f);
    p.setSource("<body style='margin:0'><div style='overflow:hidden;width:20px;height:20px'><div style='width:80px;height:80px;background:#f00'></div></div></body>", {});
    const Surface& s = p.render();
    EXPECT_TRUE(s.pixel(10, 10) == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(s.pixel(50, 50) == (Color{255, 255, 255, 255}));
}

TEST(Page, ReSetSourceResetsState) {
    Page p;
    p.setViewport(100, 50, 1.0f);
    p.setSource("<body style='background:#f00'>a</body>", {});
    p.render();
    p.setSource("<body style='background:#00f'>b</body>", {});
    EXPECT_TRUE(p.render().pixel(99, 49) == (Color{0, 0, 255, 255}));
}
