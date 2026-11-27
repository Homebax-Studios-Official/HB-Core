#include "test_support.h"
#include <hbcore/hb_core.h>
#include <hbcore/hb_core_c.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
#include "test_png_data.h"

using namespace HB;

namespace {

CoreConfig quietCfg() { CoreConfig c; c.logLevel = 5; c.useSystemFonts = false; return c; }

std::string fileUrl(const std::string& name) {
    return "file://" + std::filesystem::current_path().generic_string() + "/" + name;
}

void writeFile(const std::string& name, const std::string& content) { std::ofstream(name, std::ios::binary) << content; }

// Layout (bitmap font, 16px => 9.6 px/char, 8px body margin): the link text sits at the top-left.
const char* kPageA =
    "<html><head><title>A</title></head><body style='margin:0'>"
    "<p style='margin:0;font-size:20px'><a href='hb_in_b.html'>go</a> <a href='#sec'>jump</a> <a href='hb_in_b.html' target='_blank'>blank</a></p>"
    "<div style='height:1000px'></div><h2 id='sec' style='margin:0'>Section</h2><div style='height:600px'></div></body></html>";

struct Recorder {
    std::vector<ViewEvent> events;
    std::vector<ViewEvent> of(ViewEventType t) const { std::vector<ViewEvent> r; for (auto& e : events) if (e.type == t) r.push_back(e); return r; }
};

}  // namespace

TEST(Input, HoverCursorAndLinkAt) {
    writeFile("hb_in_a.html", kPageA);
    writeFile("hb_in_b.html", "<title>B</title><p>second");
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 400; vc.height = 200;
    View view(core, vc);
    Recorder rec;
    view.setEventHandler([&](const ViewEvent& e) { rec.events.push_back(e); });
    ASSERT_TRUE(view.loadURL(fileUrl("hb_in_a.html")).ok());
    // "go" occupies x = 0..24 at y = 0..24
    view.mouseMove(10, 10);
    EXPECT_TRUE(view.cursor() == CursorType::Hand);
    auto hovers = rec.of(ViewEventType::HoverLinkChanged);
    ASSERT_EQ(hovers.size(), size_t(1));
    EXPECT_EQ(hovers[0].text, fileUrl("hb_in_b.html"));
    EXPECT_EQ(view.linkAt(10, 10), fileUrl("hb_in_b.html"));
    view.mouseMove(10, 150);  // empty area
    EXPECT_TRUE(view.cursor() == CursorType::Arrow);
    EXPECT_EQ(rec.of(ViewEventType::HoverLinkChanged).back().text, std::string());
    EXPECT_EQ(view.linkAt(10, 150), std::string());
    std::remove("hb_in_a.html"); std::remove("hb_in_b.html");
}

TEST(Input, ClickNavigatesAndBackRestoresScroll) {
    writeFile("hb_in_a.html", kPageA);
    writeFile("hb_in_b.html", "<title>B</title><p>second");
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 400; vc.height = 200;
    View view(core, vc);
    ASSERT_TRUE(view.loadURL(fileUrl("hb_in_a.html")).ok());
    view.scrollBy(300);
    float before = view.scrollY();
    EXPECT_NEAR(before, 300, 0.01);
    // after scrolling, the link row is off-screen; scroll back to click it
    view.scrollTo(0);
    view.mouseDown(10, 10, MouseButton::Left);
    view.mouseUp(10, 10, MouseButton::Left);
    EXPECT_EQ(view.title(), std::string("B"));
    EXPECT_EQ(view.url(), fileUrl("hb_in_b.html"));
    EXPECT_TRUE(view.canGoBack());
    EXPECT_TRUE(view.goBack().ok());
    EXPECT_EQ(view.title(), std::string("A"));
    std::remove("hb_in_a.html"); std::remove("hb_in_b.html");
}

TEST(Input, PressAndReleaseOnDifferentTargetsDoesNotNavigate) {
    writeFile("hb_in_a.html", kPageA);
    writeFile("hb_in_b.html", "<title>B</title>");
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 400; vc.height = 200;
    View view(core, vc);
    view.loadURL(fileUrl("hb_in_a.html"));
    view.mouseDown(10, 10, MouseButton::Left);
    view.mouseUp(10, 150, MouseButton::Left);
    EXPECT_EQ(view.title(), std::string("A"));
    std::remove("hb_in_a.html"); std::remove("hb_in_b.html");
}

TEST(Input, FragmentLinkScrollsWithoutReload) {
    writeFile("hb_in_a.html", kPageA);
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 400; vc.height = 200;
    View view(core, vc);
    Recorder rec;
    view.setEventHandler([&](const ViewEvent& e) { rec.events.push_back(e); });
    view.loadURL(fileUrl("hb_in_a.html"));
    rec.events.clear();
    // "jump" starts after "go " (3 chars * 12 px = 36 px at 20px font)
    view.mouseDown(50, 10, MouseButton::Left);
    view.mouseUp(50, 10, MouseButton::Left);
    EXPECT_EQ(view.url(), fileUrl("hb_in_a.html") + "#sec");
    EXPECT_GT(view.scrollY(), 900);
    EXPECT_EQ(rec.of(ViewEventType::NavigationFailed).size(), size_t(0));
    EXPECT_TRUE(view.canGoBack());
    view.goBack();
    EXPECT_NEAR(view.scrollY(), 0, 0.01);
    view.loadURL(fileUrl("hb_in_a.html") + "#sec");  // fragment in the URL itself
    EXPECT_GT(view.scrollY(), 900);
    std::remove("hb_in_a.html");
}

TEST(Input, CtrlMiddleBlankAndContextMenuAreReportedToHost) {
    writeFile("hb_in_a.html", kPageA);
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 400; vc.height = 200;
    View view(core, vc);
    Recorder rec;
    view.setEventHandler([&](const ViewEvent& e) { rec.events.push_back(e); });
    view.loadURL(fileUrl("hb_in_a.html"));
    rec.events.clear();
    view.mouseDown(10, 10, MouseButton::Left);
    view.mouseUp(10, 10, MouseButton::Left, /*ctrl=*/true);
    view.mouseDown(10, 10, MouseButton::Middle);
    view.mouseUp(10, 10, MouseButton::Middle);
    view.mouseDown(100, 10, MouseButton::Left);  // target=_blank link ("blank")
    view.mouseUp(100, 10, MouseButton::Left);
    view.mouseDown(10, 10, MouseButton::Right);
    view.mouseUp(10, 10, MouseButton::Right);
    EXPECT_EQ(rec.of(ViewEventType::NewWindowRequested).size(), size_t(3));
    auto ctx = rec.of(ViewEventType::ContextMenuRequested);
    ASSERT_EQ(ctx.size(), size_t(1));
    EXPECT_EQ(ctx[0].text, fileUrl("hb_in_b.html"));
    EXPECT_NEAR(ctx[0].x, 10, 0.01);
    EXPECT_EQ(view.url(), fileUrl("hb_in_a.html"));  // nothing navigated
    std::remove("hb_in_a.html");
}

TEST(Input, WheelAndKeysScrollOnlyWhenFocused) {
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 200; vc.height = 100;
    View view(core, vc);
    view.loadHTML("<body style='margin:0'><div style='height:1000px'></div></body>");
    view.mouseWheel(120);
    EXPECT_NEAR(view.scrollY(), 120, 0.01);
    view.keyDown(Key::PageDown);
    EXPECT_NEAR(view.scrollY(), 120, 0.01);  // not focused
    view.setFocus(true);
    EXPECT_TRUE(view.hasFocus());
    view.keyDown(Key::PageDown);
    EXPECT_NEAR(view.scrollY(), 120 + 87.5f, 0.01);
    view.keyDown(Key::Down);
    EXPECT_NEAR(view.scrollY(), 120 + 87.5f + 40, 0.01);
    view.keyDown(Key::Space, /*shift=*/true);
    EXPECT_NEAR(view.scrollY(), 120 + 40, 0.01);
    view.keyDown(Key::End);
    EXPECT_NEAR(view.scrollY(), 900, 0.01);
    view.keyDown(Key::Home);
    EXPECT_NEAR(view.scrollY(), 0, 0.01);
}

TEST(Images, DataUrlPngIsRenderedAndAltIsShownWhenMissing) {
    Core core; core.initialize(quietCfg());
    ViewConfig vc; vc.width = 100; vc.height = 60;
    View view(core, vc);
    std::string html = "<body style='margin:0;font-size:10px'><img src=\"" + std::string(kRedPngDataUrl) + "\" width=20 height=20><br><img src='missing.png' alt='logo'></body>";
    ASSERT_TRUE(view.loadHTML(html).ok());
    Framebuffer fb = view.render();
    auto px = [&](int x, int y) { return fb.pixels + (y * fb.width + x) * 4; };
    // image occupies x 0..20, bottom on the baseline of the first line
    bool found = false;
    for (int y = 0; y < 30; ++y) if (px(10, y)[0] == 255 && px(10, y)[1] == 0) found = true;
    EXPECT_TRUE(found);
    // second line: alt text "[logo]" produces dark pixels
    int ink = 0;
    for (int y = 25; y < 45; ++y) for (int x = 0; x < 60; ++x) if (px(x, y)[0] < 100) ++ink;
    EXPECT_GT(ink, 20);
}

TEST(CApi, InputFunctionsAndExtendedConfig) {
    HB_CoreConfig cc = {sizeof(HB_CoreConfig), 1, 0, 256, 5, 1, nullptr};  // fonts disabled
    HB_Core* core = hb_core_create(&cc);
    ASSERT_TRUE(core != nullptr);
    HB_ViewConfig vc = {sizeof(HB_ViewConfig), 200, 100, 1.0f};
    HB_View* view = hb_view_create(core, &vc);
    ASSERT_TRUE(view != nullptr);
    EXPECT_EQ(int(hb_view_load_html(view, "<body style='margin:0'><div style='height:1000px'></div></body>")), int(HB_OK));
    hb_view_mouse_wheel(view, 50);
    EXPECT_NEAR(hb_view_get_scroll_y(view), 50, 0.01);
    hb_view_set_focus(view, 1);
    hb_view_key_down(view, HB_KEY_END, 0);
    EXPECT_NEAR(hb_view_get_scroll_y(view), 900, 0.01);
    hb_view_mouse_move(view, 5, 5);
    EXPECT_EQ(int(hb_view_get_cursor(view)), int(HB_CURSOR_ARROW));
    EXPECT_EQ(std::string(hb_view_get_link_at(view, 5, 5)), std::string());
    EXPECT_EQ(int(hb_core_load_font(core, "nonsense", 0, 0, "x.ttf")), int(HB_ERROR_INVALID_ARGUMENT));
    EXPECT_EQ(int(hb_core_load_font(core, "sans", 0, 0, "/no/such.ttf")), int(HB_ERROR_INVALID_ARGUMENT));
    hb_view_destroy(view);
    hb_core_destroy(core);
    // old-style (0.1.0-sized) config struct still works
    struct Old { uint32_t struct_size; int32_t a, b; uint32_t mem; int32_t level; } old = {sizeof(Old), 1, 0, 128, 5};
    HB_Core* c2 = hb_core_create(reinterpret_cast<HB_CoreConfig*>(&old));
    EXPECT_TRUE(c2 != nullptr);
    hb_core_destroy(c2);
}
