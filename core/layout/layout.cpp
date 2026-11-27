// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "layout/layout.hpp"
#include "base/strings.hpp"
#include "graphics/fonts.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace HB {

namespace {

float collapseMargins(float a, float b) {
    if (a >= 0 && b >= 0) return std::max(a, b);
    if (a < 0 && b < 0) return std::min(a, b);
    return a + b;
}

bool allWhitespace(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](char c) { return isAsciiSpace(c); });
}

class Builder {
public:
    Builder(const StyleResolver& r, LayoutTree& t, const ImageProvider& images) : resolver_(r), tree_(t), images_(images) {}

    LayoutBox* buildRoot(const Element& root) {
        const ComputedStyle* st = storeStyle(resolver_.compute(root, nullptr));
        return buildBox(root, st);
    }

private:
    const ComputedStyle* storeStyle(ComputedStyle s) { return tree_.arena.create<ComputedStyle>(std::move(s)); }

    LayoutBox* newBox(const Element* el, const ComputedStyle* st) {
        LayoutBox* b = tree_.arena.create<LayoutBox>();
        b->element = el;
        b->style = st;
        return b;
    }

    const ComputedStyle* anonymousStyle(const ComputedStyle& parent) {
        ComputedStyle s = parent;
        s.display = Display::Block;
        for (int i = 0; i < 4; ++i) {
            s.margin[i] = Length::px(0);
            s.padding[i] = Length::px(0);
            s.borderVisible[i] = false;
            s.borderColorSet[i] = false;
        }
        s.backgroundColor = Color::transparent();
        s.width = Length::autoLength();
        s.height = Length::autoLength();
        s.minWidth = Length::px(0);
        s.maxWidth = Length::autoLength();
        s.minHeight = Length::px(0);
        s.overflowHidden = false;
        return storeStyle(std::move(s));
    }

    static int listIndex(const Element& li) {
        int n = 1;
        const Node* parent = li.parent();
        if (parent && parent->isElement()) {
            const std::string* start = parent->asElement()->getAttribute("start");
            if (start) { char* e; long v = std::strtol(start->c_str(), &e, 10); if (e != start->c_str()) n = static_cast<int>(v); }
        }
        for (const Element* s = li.previousElementSibling(); s; s = s->previousElementSibling())
            if (s->tagName() == "li") ++n;
        return n;
    }

    static float parseDim(const std::string& v) {
        char* e = nullptr;
        float f = std::strtof(v.c_str(), &e);
        if (e == v.c_str() || (*e != '\0' && std::string(e) != "px") || f <= 0 || f > 16384) return 0;
        return f;
    }

    // <img>: decoded image atom, or "[alt]" text when the image cannot be loaded.
    void addImageItem(const Element& el, const ComputedStyle* st, std::vector<InlineItem>& run) {
        std::shared_ptr<const Image> img = images_ ? images_(el.attribute("src")) : nullptr;
        if (!img) {
            std::string alt = el.attribute("alt");
            if (!alt.empty()) run.push_back({InlineItem::Kind::Text, "[" + alt + "]", st, &el, nullptr, 0, 0});
            return;
        }
        float w = st->width.kind == Length::Kind::Px ? st->width.value : parseDim(el.attribute("width"));
        float h = st->height.kind == Length::Kind::Px ? st->height.value : parseDim(el.attribute("height"));
        float nw = static_cast<float>(img->width), nh = static_cast<float>(img->height);
        if (w > 0 && h <= 0) h = w * nh / nw;
        else if (h > 0 && w <= 0) w = h * nw / nh;
        else if (w <= 0 && h <= 0) { w = nw; h = nh; }
        InlineItem it;
        it.kind = InlineItem::Kind::Image;
        it.style = st; it.owner = &el; it.image = img; it.imageWidth = w; it.imageHeight = h;
        run.push_back(std::move(it));
    }

    LayoutBox* buildBox(const Element& el, const ComputedStyle* st) {
        LayoutBox* box = newBox(&el, st);
        std::vector<InlineItem> run;
        if (el.tagName() == "img") addImageItem(el, st, run);
        processChildren(el, *st, *box, run);
        finalize(*box, run);
        if (st->display == Display::ListItem && st->listStyle != ListStyle::None) {
            box->hasMarker = true;
            if (st->listStyle == ListStyle::Decimal) box->markerText = std::to_string(listIndex(el)) + ".";
        }
        return box;
    }

    void flushToAnonymous(LayoutBox& box, std::vector<InlineItem>& run) {
        bool onlySpace = true;
        for (const auto& it : run) {
            if (it.kind != InlineItem::Kind::Text) { onlySpace = false; break; }
            if (!allWhitespace(it.text) || it.style->preserveWhitespace) { onlySpace = false; break; }
        }
        if (!run.empty() && !onlySpace) {
            LayoutBox* anon = newBox(nullptr, anonymousStyle(*box.style));
            anon->inlineItems = std::move(run);
            box.children.push_back(anon);
        }
        run.clear();
    }

    void finalize(LayoutBox& box, std::vector<InlineItem>& run) {
        if (box.children.empty()) box.inlineItems = std::move(run);
        else flushToAnonymous(box, run);
        run.clear();
    }

    void processChildren(const Node& node, const ComputedStyle& nodeStyle, LayoutBox& box, std::vector<InlineItem>& run) {
        for (const auto& childPtr : node.children()) {
            const Node* child = childPtr.get();
            if (child->isText()) {
                run.push_back({InlineItem::Kind::Text, static_cast<const Text*>(child)->data(), &nodeStyle, node.isElement() ? node.asElement() : nullptr, nullptr, 0, 0});
                continue;
            }
            if (!child->isElement()) continue;
            const Element& el = *child->asElement();
            const ComputedStyle* st = storeStyle(resolver_.compute(el, &nodeStyle));
            if (st->display == Display::None) continue;
            if (el.tagName() == "br") { run.push_back({InlineItem::Kind::Break, "", st, &el, nullptr, 0, 0}); continue; }
            if (el.tagName() == "img" && st->display == Display::Inline) { addImageItem(el, st, run); continue; }
            if (st->display == Display::Inline) {
                processChildren(el, *st, box, run);
            } else {
                flushToAnonymous(box, run);
                box.children.push_back(buildBox(el, st));
            }
        }
    }

    const StyleResolver& resolver_;
    LayoutTree& tree_;
    const ImageProvider& images_;
};

class Layouter {
public:
    void layoutBlock(LayoutBox& b, float containing, bool isRoot) {
        const ComputedStyle& st = *b.style;
        for (int i = 0; i < 4; ++i) {
            b.padding[i] = std::max(0.0f, st.padding[i].resolve(containing, 0));
            b.border[i] = st.usedBorderWidth(i);
        }
        b.margin[0] = st.margin[0].resolve(containing, 0);
        b.margin[2] = st.margin[2].resolve(containing, 0);
        const Length& mlL = st.margin[3];
        const Length& mrL = st.margin[1];
        float ml = mlL.isAuto() ? 0 : mlL.resolve(containing);
        float mr = mrL.isAuto() ? 0 : mrL.resolve(containing);
        float extras = b.padding[1] + b.padding[3] + b.border[1] + b.border[3];
        float width = st.width.isAuto() ? containing - ml - mr - extras : st.width.resolve(containing);
        if (!st.maxWidth.isAuto()) width = std::min(width, st.maxWidth.resolve(containing));
        width = std::max(width, st.minWidth.resolve(containing, 0));
        width = std::max(0.0f, width);
        float remaining = containing - width - extras - ml - mr;
        if (mlL.isAuto() && mrL.isAuto()) { ml = mr = std::max(0.0f, remaining) / 2; }
        else if (mlL.isAuto()) ml = std::max(0.0f, remaining);
        else if (mrL.isAuto()) mr = std::max(0.0f, remaining);
        b.margin[1] = mr;
        b.margin[3] = ml;
        b.contentWidth = width;
        b.x = ml;

        bool collapseTop = !isRoot && b.padding[0] == 0 && b.border[0] == 0 && !st.overflowHidden;
        bool collapseBottom = !isRoot && b.padding[2] == 0 && b.border[2] == 0 && !st.overflowHidden && st.height.isAuto();

        float content = 0;
        if (!b.children.empty()) {
            bool first = true;
            float prevMb = 0, bottomEdge = 0;
            for (LayoutBox* child : b.children) {
                layoutBlock(*child, b.contentWidth, false);
                float cmt = child->margin[0];
                float y;
                if (first) {
                    if (collapseTop) { b.margin[0] = collapseMargins(b.margin[0], cmt); y = 0; }
                    else y = cmt;
                    first = false;
                } else {
                    y = bottomEdge + collapseMargins(prevMb, cmt);
                }
                child->y = y;
                bottomEdge = y + child->borderBoxHeight();
                prevMb = child->margin[2];
            }
            if (collapseBottom) { b.margin[2] = collapseMargins(b.margin[2], prevMb); content = bottomEdge; }
            else content = bottomEdge + prevMb;
        } else if (!b.inlineItems.empty()) {
            content = layoutInline(b);
        }
        if (!st.height.isAuto() && st.height.kind == Length::Kind::Px) content = st.height.value;
        content = std::max(content, st.minHeight.resolve(0, 0));
        b.contentHeight = std::max(0.0f, content);
    }

private:
    struct LineState {
        LineBox line;
        float x = 0;
    };

    static float textWidth(const std::string& t, const ComputedStyle* st) {
        return measureStyledText(t, st->fontSize, st->fontFamily, st->bold, st->italic);
    }
    static float spaceWidth(const ComputedStyle* st) {
        return styledAdvance(' ', st->fontSize, st->fontFamily, st->bold, st->italic);
    }

    float layoutInline(LayoutBox& b) {
        const ComputedStyle& bs = *b.style;
        const float avail = b.contentWidth;
        const float strut = bs.usedLineHeight();
        LineState cur;
        float y = 0;
        bool pendingSpace = false;
        const ComputedStyle* pendingStyle = nullptr;
        const Element* pendingOwner = nullptr;

        auto finishLine = [&](bool force) {
            if (cur.line.fragments.empty() && !force) { cur = LineState(); return; }
            float h = strut;
            float imgNeed = 0;
            const float descent = 0.2f * bs.fontSize;
            for (const auto& f : cur.line.fragments) {
                if (f.image) { imgNeed = std::max(imgNeed, f.imageHeight + descent); cur.line.hasImage = true; }
                else h = std::max(h, f.style->usedLineHeight());
            }
            h = std::max(h, imgNeed);
            cur.line.baseline = h - descent;
            float used = cur.x;
            float offset = 0;
            if (bs.textAlign == TextAlign::Center) offset = std::max(0.0f, (avail - used) / 2);
            else if (bs.textAlign == TextAlign::Right) offset = std::max(0.0f, avail - used);
            for (auto& f : cur.line.fragments) f.x += offset;
            cur.line.y = y;
            cur.line.height = h;
            y += h;
            b.lines.push_back(std::move(cur.line));
            cur = LineState();
            pendingSpace = false;
        };

        auto appendFragment = [&](const std::string& text, float w, const ComputedStyle* st, const Element* owner) {
            auto& frags = cur.line.fragments;
            if (!frags.empty() && !frags.back().image && frags.back().style == st && std::fabs(frags.back().x + frags.back().width - cur.x) < 0.01f) {
                frags.back().text += text;
                frags.back().width += w;
            } else {
                LineFragment f;
                f.x = cur.x; f.width = w; f.text = text; f.style = st; f.owner = owner;
                frags.push_back(std::move(f));
            }
            cur.x += w;
        };

        // Wrap decision shared by words and images. Returns the space width to emit before the atom.
        auto prepareAtom = [&](float w, const ComputedStyle* st, bool allowWrap) {
            bool hasContent = !cur.line.fragments.empty();
            float spaceW = (pendingSpace && hasContent) ? spaceWidth(pendingStyle) : 0;
            if (allowWrap && hasContent && cur.x + spaceW + w > avail + 0.01f) {
                finishLine(false);
                spaceW = 0;
            }
            if (spaceW > 0) appendFragment(" ", spaceW, pendingStyle, pendingOwner);
            pendingSpace = false;
            (void)st;
        };

        auto addWord = [&](const std::string& word, const ComputedStyle* st, const Element* owner) {
            if (word.empty()) return;
            float w = textWidth(word, st);
            bool wrap = !st->noWrap;
            prepareAtom(w, st, wrap);
            if (wrap && w > avail && avail > 0) {
                // Break an over-long word between characters.
                std::string piece;
                float pw = 0;
                size_t i = 0;
                while (i < word.size()) {
                    size_t before = i;
                    char32_t cp = decodeUtf8(word, i);
                    float adv = styledAdvance(cp, st->fontSize, st->fontFamily, st->bold, st->italic);
                    if (pw + adv > avail - cur.x + 0.01f && !piece.empty()) {
                        appendFragment(piece, pw, st, owner);
                        finishLine(false);
                        piece.clear(); pw = 0;
                    }
                    piece += word.substr(before, i - before);
                    pw += adv;
                }
                if (!piece.empty()) appendFragment(piece, pw, st, owner);
                return;
            }
            appendFragment(word, w, st, owner);
        };

        for (const auto& item : b.inlineItems) {
            if (item.kind == InlineItem::Kind::Break) {
                finishLine(true);
                continue;
            }
            const ComputedStyle* st = item.style;
            if (item.kind == InlineItem::Kind::Image) {
                prepareAtom(item.imageWidth, st, true);
                LineFragment f;
                f.x = cur.x; f.width = item.imageWidth; f.style = st; f.owner = item.owner;
                f.image = item.image; f.imageHeight = item.imageHeight;
                cur.line.fragments.push_back(std::move(f));
                cur.x += item.imageWidth;
                continue;
            }
            if (st->preserveWhitespace) {
                std::string seg;
                for (char c : item.text) {
                    if (c == '\r') continue;
                    if (c == '\n') { addWord(seg, st, item.owner); seg.clear(); finishLine(true); }
                    else if (c == '\t') seg += "    ";
                    else seg.push_back(c);
                }
                addWord(seg, st, item.owner);
                continue;
            }
            std::string word;
            for (char c : item.text) {
                if (isAsciiSpace(c)) {
                    if (!word.empty()) { addWord(word, st, item.owner); word.clear(); }
                    if (!cur.line.fragments.empty()) { pendingSpace = true; pendingStyle = st; pendingOwner = item.owner; }
                } else word.push_back(c);
            }
            if (!word.empty()) addWord(word, st, item.owner);
        }
        finishLine(false);
        return y;
    }
};

} // namespace

std::unique_ptr<LayoutTree> buildLayout(const Document& doc, const StyleResolver& resolver, const ViewportInfo& viewport, const ImageProvider& images) {
    auto tree = std::make_unique<LayoutTree>();
    const Element* html = doc.documentElement();
    if (!html) return tree;
    Builder builder(resolver, *tree, images);
    tree->root = builder.buildRoot(*html);

    Layouter layouter;
    layouter.layoutBlock(*tree->root, viewport.width, true);
    tree->root->y = tree->root->margin[0];
    tree->documentHeight = std::max(0.0f, tree->root->y + tree->root->borderBoxHeight() + tree->root->margin[2]);

    // Canvas background: <html> background, else <body> background (as browsers do).
    Color canvas = tree->root->style->backgroundColor;
    if (canvas.a == 0) {
        for (LayoutBox* c : tree->root->children) {
            if (c->element && c->element->tagName() == "body") canvas = c->style->backgroundColor;
        }
    }
    tree->canvasColor = canvas.a == 0 ? Color{255, 255, 255, 255} : canvas;
    return tree;
}

namespace {

const Element* linkOf(const Element* e) {
    for (const Node* n = e; n; n = n->parent())
        if (n->isElement() && n->asElement()->tagName() == "a" && n->asElement()->hasAttribute("href")) return n->asElement();
    return nullptr;
}

bool hitBox(const LayoutBox& b, float ox, float oy, float px, float py, HitTestResult& out) {
    float bx = ox + b.x, by = oy + b.y;
    float bw = b.borderBoxWidth(), bh = b.borderBoxHeight();
    float cx = bx + b.border[3] + b.padding[3], cy = by + b.border[0] + b.padding[0];
    bool inside = px >= bx && px < bx + bw && py >= by && py < by + bh;
    if (b.style->overflowHidden && !inside) return false;
    for (auto it = b.children.rbegin(); it != b.children.rend(); ++it)
        if (hitBox(**it, cx, cy, px, py, out)) return true;
    for (const auto& line : b.lines) {
        if (py < cy + line.y || py >= cy + line.y + line.height) continue;
        for (const auto& f : line.fragments) {
            if (px >= cx + f.x && px < cx + f.x + f.width) {
                out.element = f.owner ? f.owner : b.element;
                out.overText = !f.image;
                if (f.image) out.image = f.owner;
                out.link = linkOf(out.element);
                return true;
            }
        }
    }
    if (inside && b.element) {
        out.element = b.element;
        out.link = linkOf(b.element);
        return true;
    }
    return false;
}

bool findPosition(const LayoutBox& b, float ox, float oy, const Element* target, float& rx, float& ry) {
    float bx = ox + b.x, by = oy + b.y;
    if (b.element == target) { rx = bx; ry = by; return true; }
    float cx = bx + b.border[3] + b.padding[3], cy = by + b.border[0] + b.padding[0];
    for (const auto& line : b.lines)
        for (const auto& f : line.fragments)
            if (f.owner == target) { rx = cx + f.x; ry = cy + line.y; return true; }
    for (const LayoutBox* c : b.children)
        if (findPosition(*c, cx, cy, target, rx, ry)) return true;
    return false;
}

} // namespace

HitTestResult hitTest(const LayoutTree& tree, float x, float y) {
    HitTestResult r;
    if (tree.root) hitBox(*tree.root, 0, 0, x, y, r);
    return r;
}

bool elementPosition(const LayoutTree& tree, const Element* element, float& x, float& y) {
    return tree.root && element && findPosition(*tree.root, 0, 0, element, x, y);
}

} // namespace HB
