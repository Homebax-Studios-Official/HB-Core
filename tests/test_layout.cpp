#include "test_support.h"
#include "html/parser.hpp"
#include "layout/layout.hpp"
using namespace HB;

namespace {
struct Laid {
    std::unique_ptr<Document> doc;
    std::unique_ptr<Stylesheet> sheet;
    std::unique_ptr<LayoutTree> tree;
};
Laid lay(const std::string& html, const std::string& css = "", float w = 400) {
    Laid l;
    l.doc = parseHtml(html);
    l.sheet = std::make_unique<Stylesheet>(parseStylesheet(css));
    StyleResolver r(&userAgentStylesheet(), {l.sheet.get()}, ViewportInfo{w, 600});
    l.tree = buildLayout(*l.doc, r, ViewportInfo{w, 600});
    return l;
}
const LayoutBox* findBox(const LayoutBox* b, const char* tag) {
    if (b->element && b->element->tagName() == tag) return b;
    for (auto* c : b->children) if (auto* f = findBox(c, tag)) return f;
    return nullptr;
}
}

TEST(Layout, BodyMarginAndFirstChildCollapse) {
    auto l = lay("<body><h1>T</h1></body>");
    auto* body = findBox(l.tree->root, "body");
    auto* h1 = findBox(l.tree->root, "h1");
    ASSERT_TRUE(body && h1);
    // body 8px margin collapses with h1's 21.44px top margin
    EXPECT_NEAR(body->margin[0], 21.44f, 0.01f);
    EXPECT_NEAR(h1->y, 0, 0.01f);
    EXPECT_NEAR(body->contentWidth, 400 - 16, 0.01f);
    EXPECT_NEAR(h1->contentHeight, 32 * 1.2f, 0.01f);
}

TEST(Layout, SiblingMarginsCollapse) {
    auto l = lay("<body><div style='height:10px;margin-bottom:30px'></div><div style='height:10px;margin-top:20px'></div></body>");
    auto* body = findBox(l.tree->root, "body");
    ASSERT_EQ(body->children.size(), size_t(2));
    EXPECT_NEAR(body->children[1]->y - (body->children[0]->y + 10), 30, 0.01f);
}

TEST(Layout, PaddingBorderAreNotCollapsedAndAddToHeight) {
    auto l = lay("<body style='margin:0'><div style='padding:5px;border:2px solid red;height:20px'></div></body>");
    auto* div = findBox(l.tree->root, "div");
    EXPECT_NEAR(div->borderBoxHeight(), 34, 0.01f);
    EXPECT_NEAR(div->contentWidth, 400 - 14, 0.01f);
}

TEST(Layout, AutoMarginsCenterFixedWidth) {
    auto l = lay("<body style='margin:0'><div style='width:100px;margin:0 auto;height:5px'></div></body>");
    auto* div = findBox(l.tree->root, "div");
    EXPECT_NEAR(div->x, 150, 0.01f);
}

TEST(Layout, TextWrapsAtWidth) {
    auto l = lay("<body style='margin:0;font-size:10px'><p style='margin:0'>aaaa bbbb cccc dddd</p></body>", "", 60);
    auto* p = findBox(l.tree->root, "p");
    // advance is 6px/char: 10 chars per line -> "aaaa bbbb" (9 chars = 54px) then "cccc dddd"
    ASSERT_EQ(p->lines.size(), size_t(2));
    EXPECT_EQ(p->lines[0].fragments[0].text, std::string("aaaa bbbb"));
    EXPECT_EQ(p->lines[1].fragments[0].text, std::string("cccc dddd"));
    EXPECT_NEAR(p->contentHeight, 24, 0.01f);
}

TEST(Layout, LongWordIsBrokenAndBrCreatesLines) {
    auto l = lay("<body style='margin:0;font-size:10px'><p style='margin:0'>abcdefghijklmnop</p><p style='margin:0'>a<br>b<br></p></body>", "", 30);
    auto& ps = l.tree->root->children[0]->children;
    EXPECT_EQ(ps[0]->lines.size(), size_t(4));  // 5 chars per line, 16 chars
    EXPECT_EQ(ps[1]->lines.size(), size_t(2));
}

TEST(Layout, WhitespaceCollapsesAndPreKeepsNewlines) {
    auto l = lay("<body style='margin:0'><p style='margin:0'>  a \n\t b  </p><pre style='margin:0'>x\n  y</pre></body>");
    auto* p = findBox(l.tree->root, "p");
    ASSERT_EQ(p->lines.size(), size_t(1));
    EXPECT_EQ(p->lines[0].fragments[0].text, std::string("a b"));
    auto* pre = findBox(l.tree->root, "pre");
    ASSERT_EQ(pre->lines.size(), size_t(2));
    EXPECT_EQ(pre->lines[1].fragments[0].text, std::string("  y"));
}

TEST(Layout, TextAlignCenter) {
    auto l = lay("<body style='margin:0;font-size:10px'><p style='margin:0;text-align:center'>abcd</p></body>", "", 100);
    auto* p = findBox(l.tree->root, "p");
    EXPECT_NEAR(p->lines[0].fragments[0].x, (100 - 24) / 2.0f, 0.01f);
}

TEST(Layout, MixedBlockAndInlineCreatesAnonymousBox) {
    auto l = lay("<body style='margin:0'>before<div>block</div>after</body>");
    auto* body = findBox(l.tree->root, "body");
    ASSERT_EQ(body->children.size(), size_t(3));
    EXPECT_TRUE(body->children[0]->element == nullptr);
    EXPECT_TRUE(body->children[1]->element != nullptr);
}

TEST(Layout, DisplayNoneAndHeadProduceNoBoxes) {
    auto l = lay("<head><title>x</title></head><body><div style='display:none'>hidden</div><p>shown</p></body>");
    EXPECT_TRUE(findBox(l.tree->root, "title") == nullptr);
    auto* body = findBox(l.tree->root, "body");
    EXPECT_EQ(body->children.size(), size_t(1));
}

TEST(Layout, ListMarkersAndNumbering) {
    auto l = lay("<body><ol start=3><li>a</li><li>b</li></ol><ul><li>c</li></ul></body>");
    auto* ol = findBox(l.tree->root, "ol");
    ASSERT_EQ(ol->children.size(), size_t(2));
    EXPECT_EQ(ol->children[0]->markerText, std::string("3."));
    EXPECT_EQ(ol->children[1]->markerText, std::string("4."));
    auto* ul = findBox(l.tree->root, "ul");
    EXPECT_TRUE(ul->children[0]->hasMarker);
    EXPECT_EQ(ul->children[0]->markerText, std::string());
}

TEST(Layout, CanvasColorFromBody) {
    auto l = lay("<body style='background:#112233'>x</body>");
    EXPECT_TRUE(l.tree->canvasColor == (Color{0x11, 0x22, 0x33, 255}));
}

TEST(Layout, EmptyDocumentHasZeroishHeight) {
    auto l = lay("");
    EXPECT_GE(l.tree->documentHeight, 0.0f);
}
