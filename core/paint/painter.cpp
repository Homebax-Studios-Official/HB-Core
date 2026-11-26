// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "paint/display_list.hpp"
#include "graphics/bitmap_font.hpp"
#include <algorithm>

namespace HB {

namespace {

Color withOpacity(Color c, float opacity) {
    c.a = static_cast<uint8_t>(c.a * opacity + 0.5f);
    return c;
}

void addRect(DisplayList& l, float x, float y, float w, float h, Color c) {
    if (w <= 0 || h <= 0 || c.a == 0) return;
    DisplayItem it;
    it.type = DisplayItem::Type::FillRect;
    it.x = x; it.y = y; it.w = w; it.h = h; it.color = c;
    l.items.push_back(std::move(it));
}

TextStyle toTextStyle(const ComputedStyle& s) {
    TextStyle t;
    t.fontSize = s.fontSize;
    t.bold = s.bold;
    t.italic = s.italic;
    t.underline = s.underline;
    t.lineThrough = s.lineThrough;
    t.color = withOpacity(s.color, s.opacity);
    return t;
}

void paintBox(const LayoutBox& b, float originX, float originY, DisplayList& list) {
    const ComputedStyle& st = *b.style;
    float bx = originX + b.x, by = originY + b.y;
    float bw = b.borderBoxWidth(), bh = b.borderBoxHeight();
    if (st.visible) {
        addRect(list, bx, by, bw, bh, withOpacity(st.backgroundColor, st.opacity));
        if (b.border[0] > 0) addRect(list, bx, by, bw, b.border[0], withOpacity(st.usedBorderColor(0), st.opacity));
        if (b.border[2] > 0) addRect(list, bx, by + bh - b.border[2], bw, b.border[2], withOpacity(st.usedBorderColor(2), st.opacity));
        if (b.border[3] > 0) addRect(list, bx, by + b.border[0], b.border[3], bh - b.border[0] - b.border[2], withOpacity(st.usedBorderColor(3), st.opacity));
        if (b.border[1] > 0) addRect(list, bx + bw - b.border[1], by + b.border[0], b.border[1], bh - b.border[0] - b.border[2], withOpacity(st.usedBorderColor(1), st.opacity));
    }
    float cx = bx + b.border[3] + b.padding[3];
    float cy = by + b.border[0] + b.padding[0];

    if (st.overflowHidden) {
        DisplayItem clip;
        clip.type = DisplayItem::Type::PushClip;
        clip.x = bx + b.border[3]; clip.y = by + b.border[0];
        clip.w = bw - b.border[3] - b.border[1]; clip.h = bh - b.border[0] - b.border[2];
        list.items.push_back(std::move(clip));
    }

    if (b.hasMarker && st.visible) {
        float lh = st.usedLineHeight();
        float midY = cy + lh / 2;
        Color c = withOpacity(st.color, st.opacity);
        if (st.listStyle == ListStyle::Decimal) {
            DisplayItem t;
            t.type = DisplayItem::Type::DrawText;
            t.textStyle = toTextStyle(st);
            t.textStyle.underline = false;
            t.textStyle.lineThrough = false;
            t.text = b.markerText;
            float w = measureText(b.markerText, st.fontSize);
            t.x = cx - w - st.fontSize * 0.4f;
            t.y = midY - st.fontSize / 2;
            list.items.push_back(std::move(t));
        } else if (st.listStyle == ListStyle::Square) {
            float s = st.fontSize * 0.3f;
            addRect(list, cx - st.fontSize * 0.9f, midY - s / 2, s, s, c);
        } else {
            DisplayItem d;
            d.type = DisplayItem::Type::FillCircle;
            d.x = cx - st.fontSize * 0.75f; d.y = midY; d.w = st.fontSize * 0.17f; d.color = c;
            list.items.push_back(std::move(d));
        }
    }

    if (st.visible) {
        for (const auto& line : b.lines) {
            for (const auto& f : line.fragments) {
                const ComputedStyle& fs = *f.style;
                if (!fs.visible) continue;
                float fy = cy + line.y + (line.height - fs.fontSize) / 2;
                if (fs.display == Display::Inline && fs.backgroundColor.a > 0) {
                    addRect(list, cx + f.x, fy - fs.fontSize * 0.1f, f.width, fs.fontSize * 1.2f, withOpacity(fs.backgroundColor, fs.opacity));
                }
                DisplayItem t;
                t.type = DisplayItem::Type::DrawText;
                t.x = cx + f.x; t.y = fy; t.text = f.text;
                t.textStyle = toTextStyle(fs);
                list.items.push_back(std::move(t));
            }
        }
    }

    for (const LayoutBox* c : b.children) paintBox(*c, cx, cy, list);

    if (st.overflowHidden) {
        DisplayItem pop;
        pop.type = DisplayItem::Type::PopClip;
        list.items.push_back(std::move(pop));
    }
}

} // namespace

DisplayList buildDisplayList(const LayoutTree& tree) {
    DisplayList list;
    list.canvasColor = tree.canvasColor;
    list.documentHeight = tree.documentHeight;
    if (tree.root) paintBox(*tree.root, 0, 0, list);
    return list;
}

void rasterize(const DisplayList& list, Surface& surface, float scrollY, float scale) {
    surface.clear(list.canvasColor);
    const float viewH = static_cast<float>(surface.height());
    for (const auto& it : list.items) {
        switch (it.type) {
            case DisplayItem::Type::PushClip:
                surface.pushClip(it.x * scale, (it.y - scrollY) * scale, it.w * scale, it.h * scale);
                break;
            case DisplayItem::Type::PopClip:
                surface.popClip();
                break;
            case DisplayItem::Type::FillRect: {
                float y = (it.y - scrollY) * scale, h = it.h * scale;
                if (y > viewH || y + h < 0) break;
                surface.fillRect(it.x * scale, y, it.w * scale, h, it.color);
                break;
            }
            case DisplayItem::Type::FillCircle: {
                float y = (it.y - scrollY) * scale;
                if (y - it.w * scale > viewH || y + it.w * scale < 0) break;
                surface.fillCircle(it.x * scale, y, it.w * scale, it.color);
                break;
            }
            case DisplayItem::Type::DrawText: {
                float y = (it.y - scrollY) * scale;
                float fs = it.textStyle.fontSize * scale;
                if (y > viewH || y + fs < 0) break;
                TextStyle ts = it.textStyle;
                ts.fontSize = fs;
                surface.drawText(it.x * scale, y, it.text, ts);
                break;
            }
        }
    }
}

} // namespace HB
