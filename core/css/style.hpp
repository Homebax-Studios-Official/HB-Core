// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "css/color.hpp"
#include "css/font_family.hpp"
#include "css/stylesheet.hpp"
#include "dom/node.hpp"

namespace HB {

struct Length {
    enum class Kind { Auto, Px, Percent };
    Kind kind = Kind::Auto;
    float value = 0;
    static Length px(float v) { return {Kind::Px, v}; }
    static Length percent(float v) { return {Kind::Percent, v}; }
    static Length autoLength() { return {Kind::Auto, 0}; }
    bool isAuto() const { return kind == Kind::Auto; }
    // Resolves against a containing size; returns fallback for auto.
    float resolve(float containing, float fallback = 0) const {
        if (kind == Kind::Px) return value;
        if (kind == Kind::Percent) return containing * value / 100.0f;
        return fallback;
    }
};

enum class Display { None, Block, Inline, ListItem };
enum class TextAlign { Left, Center, Right };
enum class ListStyle { Disc, Circle, Square, Decimal, None };

struct ComputedStyle {
    ComputedStyle() {
        for (int i = 0; i < 4; ++i) { margin[i] = Length::px(0); padding[i] = Length::px(0); }
    }
    Display display = Display::Inline;
    Color color{0, 0, 0, 255};
    Color backgroundColor = Color::transparent();
    float fontSize = 16;
    FontFamily fontFamily = FontFamily::Sans;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool lineThrough = false;
    TextAlign textAlign = TextAlign::Left;
    enum class LineHeightMode { Normal, Number, Px } lineHeightMode = LineHeightMode::Normal;
    float lineHeightValue = 1.2f;
    bool preserveWhitespace = false;  // white-space: pre | pre-wrap | pre-line (newlines kept)
    bool noWrap = false;
    ListStyle listStyle = ListStyle::Disc;
    bool visible = true;
    bool overflowHidden = false;
    float opacity = 1.0f;

    Length margin[4];   // top, right, bottom, left
    Length padding[4];
    float borderWidth[4] = {3, 3, 3, 3};
    bool borderVisible[4] = {false, false, false, false};  // border-style != none/hidden
    Color borderColor[4];
    bool borderColorSet[4] = {false, false, false, false};
    Length width, height;
    Length minWidth = Length::px(0);
    Length maxWidth;  // auto = none
    Length minHeight = Length::px(0);

    std::map<std::string, std::string> customProperties;

    float usedLineHeight() const {
        switch (lineHeightMode) {
            case LineHeightMode::Px: return lineHeightValue;
            case LineHeightMode::Number: return lineHeightValue * fontSize;
            default: return 1.2f * fontSize;
        }
    }
    float usedBorderWidth(int side) const { return borderVisible[side] ? borderWidth[side] : 0.0f; }
    Color usedBorderColor(int side) const { return borderColorSet[side] ? borderColor[side] : color; }
};

struct ViewportInfo {
    float width = 800;
    float height = 600;
};

// Computes styles from the cascade. Stylesheet pointers must outlive the resolver.
class StyleResolver {
public:
    StyleResolver(const Stylesheet* userAgent, std::vector<const Stylesheet*> author, ViewportInfo viewport)
        : ua_(userAgent), author_(std::move(author)), viewport_(viewport) {}

    // parent may be nullptr for the root element.
    ComputedStyle compute(const Element& el, const ComputedStyle* parent) const;

private:
    const Stylesheet* ua_;
    std::vector<const Stylesheet*> author_;
    ViewportInfo viewport_;
};

// Exposed for tests.
bool parseLengthValue(const std::string& text, float fontSize, float rootFontSize, const ViewportInfo& vp, Length& out);
const Stylesheet& userAgentStylesheet();

} // namespace HB
