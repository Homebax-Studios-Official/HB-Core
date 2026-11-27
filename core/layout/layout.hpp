// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "base/arena.hpp"
#include "css/style.hpp"
#include "dom/node.hpp"
#include "graphics/image.hpp"

namespace HB {

struct InlineItem {
    enum class Kind { Text, Break, Image } kind = Kind::Text;
    std::string text;
    const ComputedStyle* style = nullptr;
    const Element* owner = nullptr;            // element the text/image belongs to (for hit testing)
    std::shared_ptr<const Image> image;        // Kind::Image
    float imageWidth = 0, imageHeight = 0;     // CSS px
};

struct LineFragment {
    float x = 0;      // relative to the content box of the owning block
    float width = 0;
    std::string text;
    const ComputedStyle* style = nullptr;
    const Element* owner = nullptr;
    std::shared_ptr<const Image> image;        // non-null for image fragments (text is empty)
    float imageHeight = 0;
};

struct LineBox {
    float y = 0;       // relative to the content box top of the owning block
    float height = 0;
    bool hasImage = false;
    float baseline = 0;  // relative to the line top; only meaningful when hasImage
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

// Resolves an <img src> to decoded pixels (nullptr if unavailable).
using ImageProvider = std::function<std::shared_ptr<const Image>(const std::string& src)>;

struct HitTestResult {
    const Element* element = nullptr;  // deepest element under the point
    const Element* link = nullptr;     // nearest <a href> ancestor-or-self
    const Element* image = nullptr;    // <img> under the point
    bool overText = false;
};

// Computes styles and builds + lays out the box tree for the document at the given viewport width.
//
// Implemented: block flow with margin collapsing (siblings, parent/first child, parent/last child),
// anonymous block boxes, inline text flow with wrapping, <br>, <img> as an inline atom, white-space, text-align, list markers,
// width/min/max-width, auto side margins, padding, borders.
// NOT implemented: floats, positioning, flexbox/grid/tables (treated as block), inline-block,
// inline margins/padding/borders, vertical-align, bidi, hyphenation. See SUPPORT_MATRIX.md.
std::unique_ptr<LayoutTree> buildLayout(const Document& doc, const StyleResolver& resolver, const ViewportInfo& viewport, const ImageProvider& images = ImageProvider());

// Point in document coordinates (CSS px).
HitTestResult hitTest(const LayoutTree& tree, float x, float y);
// Absolute top-left of the element's first box or line fragment, for fragment navigation.
bool elementPosition(const LayoutTree& tree, const Element* element, float& x, float& y);

} // namespace HB
