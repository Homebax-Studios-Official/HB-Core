#include "test_support.h"
#include <hbcore/hb_core.h>
#include <hbcore/hb_core_c.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace HB;

namespace {
CoreConfig quiet() { CoreConfig c; c.logLevel = 5; return c; }
const char* kPage = "<html><head><title>T1</title></head><body><h1>Hi</h1></body></html>";
}

TEST(Api, CoreLifecycle) {
    Core core;
    EXPECT_FALSE(core.isInitialized());
    EXPECT_TRUE(core.initialize(quiet()).ok());
    EXPECT_TRUE(core.isInitialized());
    EXPECT_FALSE(core.initialize(quiet()).ok());
    CoreConfig bad = quiet(); bad.memoryLimitMB = 0;
    Core c2;
    EXPECT_FALSE(c2.initialize(bad).ok());
    EXPECT_EQ(std::string(Core::version()), std::string("0.1.0"));
}

TEST(Api, LoadHtmlEventsTitleAndHistory) {
    Core core; core.initialize(quiet());
    View view(core);
    std::vector<ViewEventType> events;
    view.setEventHandler([&](const ViewEvent& e) { events.push_back(e.type); });
    EXPECT_TRUE(view.loadHTML(kPage).ok());
    EXPECT_EQ(view.title(), std::string("T1"));
    ASSERT_GE(events.size(), size_t(3));
    EXPECT_TRUE(events.front() == ViewEventType::NavigationStarted);
    EXPECT_TRUE(events.back() == ViewEventType::NavigationCompleted);
    EXPECT_FALSE(view.canGoBack());
    EXPECT_TRUE(view.loadHTML("<title>T2</title><p>x").ok());
    EXPECT_EQ(view.title(), std::string("T2"));
    EXPECT_TRUE(view.canGoBack());
    EXPECT_TRUE(view.goBack().ok());
    EXPECT_EQ(view.title(), std::string("T1"));
    EXPECT_TRUE(view.canGoForward());
    EXPECT_TRUE(view.goForward().ok());
    EXPECT_EQ(view.title(), std::string("T2"));
    EXPECT_FALSE(view.goForward().ok());
    EXPECT_TRUE(view.reload().ok());
}

TEST(Api, DataUrlAndAboutBlank) {
    Core core; core.initialize(quiet());
    View view(core);
    EXPECT_TRUE(view.loadURL("data:text/html,%3Ctitle%3EData%3C/title%3E%3Cp%3Eok").ok());
    EXPECT_EQ(view.title(), std::string("Data"));
    EXPECT_TRUE(view.loadURL("about:blank").ok());
    EXPECT_EQ(view.title(), std::string(""));
    EXPECT_EQ(view.url(), std::string("about:blank"));
}

TEST(Api, FileUrlLoadsAndRelativeStylesheetApplies) {
    { std::ofstream f("hb_test_page.html"); f << "<head><title>File</title><link rel=stylesheet href=\"hb_test.css\"></head><body style='margin:0'>x</body>"; }
    { std::ofstream f("hb_test.css"); f << "body{background:#00ff00}"; }
    Core core; core.initialize(quiet());
    ViewConfig vc; vc.width = 50; vc.height = 50;
    View view(core, vc);
    std::string path = "hb_test_page.html";
    Error e = view.loadURL("file://" + std::filesystem::current_path().generic_string() + "/" + path);
    EXPECT_TRUE(e.ok());
    EXPECT_EQ(view.title(), std::string("File"));
    Framebuffer fb = view.render();
    ASSERT_TRUE(fb.pixels != nullptr);
    EXPECT_EQ(int(fb.pixels[(40 * fb.width + 40) * 4 + 1]), 255);
    std::remove("hb_test_page.html");
    std::remove("hb_test.css");
}

TEST(Api, FailuresAreReportedHonestly) {
    Core core; core.initialize(quiet());
    View view(core);
    int failures = 0;
    view.setEventHandler([&](const ViewEvent& e) { if (e.type == ViewEventType::NavigationFailed) ++failures; });
    Error net = view.loadURL("https://example.com");
    EXPECT_TRUE(net.code == ErrorCode::Unsupported);
    Error missing = view.loadURL("file:///definitely/not/here.html");
    EXPECT_TRUE(missing.code == ErrorCode::NotFound);
    EXPECT_FALSE(view.loadURL("").ok());
    EXPECT_FALSE(view.loadURL("ftp://x").ok());
    EXPECT_EQ(failures, 4);
    std::string out = "x";
    Error js = view.executeJavaScript("document.title", &out);
    EXPECT_TRUE(js.code == ErrorCode::Unsupported);
    EXPECT_EQ(out, std::string());
    EXPECT_EQ(view.url(), std::string("about:blank"));
}

TEST(Api, MemoryLimitRejectsHugeDocument) {
    Core core;
    CoreConfig c = quiet(); c.memoryLimitMB = 1;  // 128 KiB of source
    core.initialize(c);
    View view(core);
    std::string big(200 * 1024, 'a');
    EXPECT_TRUE(view.loadHTML(big).code == ErrorCode::InvalidArgument);
}

TEST(Api, ViewNeedsInitializedCore) {
    Core core;
    View view(core);
    EXPECT_FALSE(view.loadHTML("x").ok());
}

TEST(Api, ResizeScrollAndDocumentHeight) {
    Core core; core.initialize(quiet());
    View view(core);
    std::string html = "<body style='margin:0'>";
    for (int i = 0; i < 30; ++i) html += "<div style='height:50px'></div>";
    view.loadHTML(html);
    view.resize(300, 200);
    EXPECT_NEAR(view.documentHeight(), 1500, 0.01);
    view.scrollBy(120);
    EXPECT_NEAR(view.scrollY(), 120, 0.01);
    view.scrollBy(100000);
    EXPECT_NEAR(view.scrollY(), 1300, 0.01);
    view.scrollTo(-1);
    EXPECT_NEAR(view.scrollY(), 0, 0.01);
    view.setDevicePixelRatio(2.0f);
    Framebuffer fb = view.render();
    EXPECT_EQ(fb.width, 600);
    EXPECT_EQ(fb.height, 400);
}

TEST(CApi, FullFlow) {
    HB_CoreConfig cc = {sizeof(HB_CoreConfig), 1, 0, 256, 5};
    HB_Core* core = hb_core_create(&cc);
    ASSERT_TRUE(core != nullptr);
    HB_ViewConfig vc = {sizeof(HB_ViewConfig), 120, 80, 1.0f};
    HB_View* view = hb_view_create(core, &vc);
    ASSERT_TRUE(view != nullptr);
    static int titleEvents;
    titleEvents = 0;
    hb_view_set_event_callback(view, [](const HB_Event* e, void*) { if (e->type == HB_EVENT_TITLE_CHANGED) ++titleEvents; }, nullptr);
    EXPECT_EQ(int(hb_view_load_html(view, "<title>CT</title><body style='margin:0;background:#0000ff'>x")), int(HB_OK));
    EXPECT_EQ(std::string(hb_view_get_title(view)), std::string("CT"));
    EXPECT_EQ(titleEvents, 1);
    int w = 0, h = 0, stride = 0;
    const uint8_t* px = hb_view_get_pixels(view, &w, &h, &stride);
    ASSERT_TRUE(px != nullptr);
    EXPECT_EQ(w, 120); EXPECT_EQ(h, 80); EXPECT_EQ(stride, 480);
    EXPECT_EQ(int(px[(70 * stride) + 100 * 4 + 2]), 255);
    hb_view_resize(view, 60, 40);
    px = hb_view_get_pixels(view, &w, &h, &stride);
    EXPECT_EQ(w, 60);
    EXPECT_EQ(int(hb_view_execute_javascript(view, "1")), int(HB_ERROR_UNSUPPORTED));
    EXPECT_EQ(int(hb_view_load_url(view, "http://example.com")), int(HB_ERROR_UNSUPPORTED));
    EXPECT_EQ(int(hb_view_load_html(nullptr, "x")), int(HB_ERROR_INVALID_ARGUMENT));
    EXPECT_EQ(int(hb_view_save_png(view, "hb_capi_test.png")), int(HB_OK));
    std::ifstream png("hb_capi_test.png", std::ios::binary);
    EXPECT_TRUE(png.good());
    png.close();
    std::remove("hb_capi_test.png");
    EXPECT_EQ(hb_abi_version(), HB_ABI_VERSION);
    hb_view_destroy(view);
    hb_core_destroy(core);
    hb_view_destroy(nullptr);
    hb_core_destroy(nullptr);
}

TEST(CApi, NullArgumentsAreSafe) {
    EXPECT_TRUE(hb_view_create(nullptr, nullptr) == nullptr);
    EXPECT_EQ(std::string(hb_view_get_title(nullptr)), std::string());
    EXPECT_TRUE(hb_view_get_pixels(nullptr, nullptr, nullptr, nullptr) == nullptr);
    hb_view_resize(nullptr, 1, 1);
    hb_view_scroll_by(nullptr, 1);
    HB_Core* core = hb_core_create(nullptr);  // defaults
    EXPECT_TRUE(core != nullptr);
    hb_core_destroy(core);
}
