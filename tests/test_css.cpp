#include "test_support.h"
#include "css/color.hpp"
#include "css/style.hpp"
#include "css/stylesheet.hpp"
#include "html/parser.hpp"
using namespace HB;

TEST(Color, Formats) {
    EXPECT_TRUE(*parseColor("#f00") == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(*parseColor("#00ff0080") == (Color{0, 255, 0, 128}));
    EXPECT_TRUE(*parseColor("rgb(1, 2, 3)") == (Color{1, 2, 3, 255}));
    EXPECT_TRUE(*parseColor("rgba(255 0 0 / 50%)") == (Color{255, 0, 0, 128}));
    EXPECT_TRUE(*parseColor("hsl(120, 100%, 50%)") == (Color{0, 255, 0, 255}));
    EXPECT_TRUE(*parseColor("Navy") == (Color{0, 0, 128, 255}));
    EXPECT_TRUE(parseColor("transparent")->a == 0);
    EXPECT_FALSE(parseColor("#12").has_value());
    EXPECT_FALSE(parseColor("notacolor").has_value());
    EXPECT_FALSE(parseColor("rgb(1,2)").has_value());
}

TEST(Css, ParsesRulesAndDeclarations) {
    auto s = parseStylesheet("/* c */ h1, .a > p { color: red; margin: 0 auto !important } @import url(x); @font-face { src: url(y) } b{font-weight:bold}");
    ASSERT_EQ(s.rules.size(), size_t(2));
    EXPECT_EQ(s.rules[0].selectors.size(), size_t(2));
    ASSERT_EQ(s.rules[0].declarations.size(), size_t(2));
    EXPECT_TRUE(s.rules[0].declarations[1].important);
    EXPECT_EQ(s.rules[0].declarations[1].value, std::string("0 auto"));
}

TEST(Css, InvalidSelectorDropsRule) {
    auto s = parseStylesheet("a, {color:red} p{color:blue} >x{color:red}");
    ASSERT_EQ(s.rules.size(), size_t(1));
}

TEST(Css, SelectorMatchingAndSpecificity) {
    auto doc = parseHtml("<body><div id=m class='a b'><p>1</p><p class=t>2</p><span>3</span></div></body>");
    auto ps = doc->getElementsByTagName("p");
    auto sp = doc->getElementsByTagName("span");
    auto match = [](const char* sel, const Element& e) {
        auto l = parseSelectorList(sel);
        for (auto& s : l) if (selectorMatches(s, e)) return true;
        return false;
    };
    EXPECT_TRUE(match("div p", *ps[0]));
    EXPECT_TRUE(match("div > p.t", *ps[1]));
    EXPECT_FALSE(match("body > p", *ps[0]));
    EXPECT_TRUE(match("p + p", *ps[1]));
    EXPECT_TRUE(match("p ~ span", *sp[0]));
    EXPECT_TRUE(match("#m.a.b", *doc->getElementById("m")));
    EXPECT_TRUE(match("[class~=b]", *doc->getElementById("m")));
    EXPECT_TRUE(match("p:first-child", *ps[0]));
    EXPECT_FALSE(match("p:first-child", *ps[1]));
    EXPECT_TRUE(match("span:last-child", *sp[0]));
    EXPECT_FALSE(match("a:hover", *ps[0]));
    EXPECT_GT(parseSelectorList("#a")[0].specificity, parseSelectorList(".a.b.c")[0].specificity);
    EXPECT_GT(parseSelectorList(".a")[0].specificity, parseSelectorList("div p")[0].specificity);
}

namespace {
ComputedStyle styleOf(const std::string& html, const std::string& css, const char* id, float vw = 800) {
    static std::vector<std::unique_ptr<Document>> keep;
    static std::vector<std::unique_ptr<Stylesheet>> sheets;
    keep.push_back(parseHtml(html));
    sheets.push_back(std::make_unique<Stylesheet>(parseStylesheet(css)));
    StyleResolver r(&userAgentStylesheet(), {sheets.back().get()}, ViewportInfo{vw, 600});
    // compute along the ancestor chain so inheritance works
    std::vector<const Element*> chain;
    for (const Node* n = keep.back()->getElementById(id); n && n->isElement(); n = n->parent()) chain.insert(chain.begin(), n->asElement());
    ComputedStyle cur;
    const ComputedStyle* parent = nullptr;
    for (auto* e : chain) { cur = r.compute(*e, parent); parent = &cur; static std::vector<std::unique_ptr<ComputedStyle>> hold; hold.push_back(std::make_unique<ComputedStyle>(cur)); parent = hold.back().get(); }
    return cur;
}
}

TEST(Style, UserAgentDefaults) {
    auto s = styleOf("<body><h1 id=h>x</h1></body>", "", "h");
    EXPECT_TRUE(s.display == Display::Block);
    EXPECT_TRUE(s.bold);
    EXPECT_NEAR(s.fontSize, 32, 0.01);
    EXPECT_NEAR(s.margin[0].value, 21.44, 0.01);
}

TEST(Style, CascadeSpecificityOrderImportant) {
    const char* html = "<body><p id=p class=c style='color:#00f'>x</p></body>";
    EXPECT_TRUE(styleOf(html, "p{color:red} .c{color:green}", "p").color == (Color{0, 0, 255, 255}));
    EXPECT_TRUE(styleOf(html, ".c{color:green !important}", "p").color == (Color{0, 128, 0, 255}));
    EXPECT_TRUE(styleOf("<body><p id=p class=c>x</p></body>", "p{color:red} .c{color:green} p{color:blue}", "p").color == (Color{0, 128, 0, 255}));
    EXPECT_TRUE(styleOf("<body><p id=p>x</p></body>", "p{color:red} p{color:blue}", "p").color == (Color{0, 0, 255, 255}));
}

TEST(Style, InheritanceAndUnits) {
    auto s = styleOf("<body><div style='font-size:20px;color:red'><p id=p style='margin-left:2em;padding:10px 5px;width:50%'>x</p></div></body>", "", "p");
    EXPECT_NEAR(s.fontSize, 20, 0.01);
    EXPECT_TRUE(s.color == (Color{255, 0, 0, 255}));
    EXPECT_NEAR(s.margin[3].value, 40, 0.01);
    EXPECT_NEAR(s.padding[0].value, 10, 0.01);
    EXPECT_NEAR(s.padding[1].value, 5, 0.01);
    EXPECT_TRUE(s.width.kind == Length::Kind::Percent);
}

TEST(Style, CustomPropertiesAndVar) {
    auto s = styleOf("<body><div style='--c:#0f0'><p id=p style='color:var(--c);background:var(--missing, #00f)'>x</p></div></body>", "", "p");
    EXPECT_TRUE(s.color == (Color{0, 255, 0, 255}));
    EXPECT_TRUE(s.backgroundColor == (Color{0, 0, 255, 255}));
}

TEST(Style, MediaQueriesUseViewportWidth) {
    const char* css = "p{color:red} @media (max-width: 500px){p{color:blue}} @media (min-width: 900px){p{color:green}} @media print{p{color:black}}";
    const char* html = "<body><p id=p>x</p></body>";
    EXPECT_TRUE(styleOf(html, css, "p", 400).color == (Color{0, 0, 255, 255}));
    EXPECT_TRUE(styleOf(html, css, "p", 700).color == (Color{255, 0, 0, 255}));
    EXPECT_TRUE(styleOf(html, css, "p", 1000).color == (Color{0, 128, 0, 255}));
}

TEST(Style, BorderShorthandAndDisplayNone) {
    auto s = styleOf("<body><div id=d style='border:2px solid #333;display:none'>x</div></body>", "", "d");
    EXPECT_TRUE(s.display == Display::None);
    EXPECT_TRUE(s.borderVisible[0]);
    EXPECT_NEAR(s.usedBorderWidth(3), 2, 0.01);
    auto n = styleOf("<body><div id=d style='border:2px'>x</div></body>", "", "d");
    EXPECT_NEAR(n.usedBorderWidth(0), 0, 0.01);
}

TEST(Style, LengthParsing) {
    Length l;
    ViewportInfo vp{1000, 500};
    EXPECT_TRUE(parseLengthValue("10vw", 16, 16, vp, l));
    EXPECT_NEAR(l.value, 100, 0.01);
    EXPECT_TRUE(parseLengthValue("1.5rem", 10, 16, vp, l));
    EXPECT_NEAR(l.value, 24, 0.01);
    EXPECT_TRUE(parseLengthValue("0", 16, 16, vp, l));
    EXPECT_FALSE(parseLengthValue("calc(1px + 2px)", 16, 16, vp, l));
    EXPECT_FALSE(parseLengthValue("12", 16, 16, vp, l));
}
