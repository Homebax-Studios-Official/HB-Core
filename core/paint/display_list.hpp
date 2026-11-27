// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "graphics/surface.hpp"
#include "layout/layout.hpp"

namespace HB {

struct DisplayItem {
    enum class Type { FillRect, FillCircle, DrawText, DrawImage, PushClip, PopClip } type = Type::FillRect;
    float x = 0, y = 0, w = 0, h = 0;  // CSS pixels, document coordinates (circle: x,y = centre, w = radius)
    Color color;
    std::string text;
    TextStyle textStyle;
    std::shared_ptr<const Image> image;  // DrawImage; opacity in color.a
};

struct DisplayList {
    std::vector<DisplayItem> items;
    Color canvasColor{255, 255, 255, 255};
    float documentHeight = 0;
};

// Walks the layout tree in paint order (backgrounds, borders, then content per box).
// NOT implemented: stacking contexts, z-index, opacity groups, transforms, box shadows, border radius.
DisplayList buildDisplayList(const LayoutTree& tree);

// Rasterizes the display list. `scrollY` is in CSS pixels; `scale` is the device pixel ratio.
void rasterize(const DisplayList& list, Surface& surface, float scrollY, float scale);

} // namespace HB
