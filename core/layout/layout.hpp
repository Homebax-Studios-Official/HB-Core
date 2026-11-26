// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "base/arena.hpp"
#include "css/style.hpp"
#include "dom/node.hpp"

namespace HB {

struct InlineItem {
    enum class Kind { Text, Break } kind = Kind::Text;
    std::string text;
    const ComputedStyle* style = nullptr;
};

struct LineFragment {
    float x = 0;      // relative to the content box of the owning block
    float width = 0;
    std::string text;
    const ComputedStyle* style = nullptr;
};

struct LineBox {
    float y = 0;       // relative to the content box top of the owning block
    float height = 0;
    std::vector<LineFragment> fragments;
};

// Edge order everywhere: top, right, bottom, left.
struct LayoutBox {
    const Element* element = nullptr;  // nullptr for anonymous boxes
    const ComputedStyle* style = nullptr;
    std::vector<LayoutBox*> children;
    std::vector<InlineItem> inlineItems;  // only when children is empty
    std::vector<LineBox> lines;

    bool hasMarker = false;
    std::string markerText;  // for decimal lists

    float x = 0, y = 0;  // border-box origin relative to the parent's content box
    float margin[4] = {0, 0, 0, 0};
    float border[4] = {0, 0, 0, 0};
    float padding[4] = {0, 0, 0, 0};
    float contentWidth = 0, contentHeight = 0;

    float borderBoxWidth() const { return contentWidth + padding[1] + padding[3] + border[1] + border[3]; }
    float borderBoxHeight() const { return contentHeight + padding[0] + padding[2] + border[0] + border[2]; }
};

class LayoutTree {
public:
    LayoutBox* root = nullptr;
    float documentHeight = 0;
    Color canvasColor{255, 255, 255, 255};
    Arena arena;
};

// Computes styles and builds + lays out the box tree for the document at the given viewport width.
//
// Implemented: block flow with margin collapsing (siblings, parent/first child, parent/last child),
// anonymous block boxes, inline text flow with wrapping, <br>, white-space, text-align, list markers,
// width/min/max-width, auto side margins, padding, borders.
// NOT implemented: floats, positioning, flexbox/grid/tables (treated as block), inline-block,
// inline margins/padding/borders, images, vertical-align, bidi, hyphenation. See SUPPORT_MATRIX.md.
std::unique_ptr<LayoutTree> buildLayout(const Document& doc, const StyleResolver& resolver, const ViewportInfo& viewport);

} // namespace HB
