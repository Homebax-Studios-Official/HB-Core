// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "layout/layout.hpp"
#include "base/strings.hpp"
#include "graphics/bitmap_font.hpp"
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
    Builder(const StyleResolver& r, LayoutTree& t) : resolver_(r), tree_(t) {}

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

    LayoutBox* buildBox(const Element& el, const ComputedStyle* st) {
        LayoutBox* box = newBox(&el, st);
        std::vector<InlineItem> run;
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
            if (it.kind == InlineItem::Kind::Break) { onlySpace = false; break; }
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
                run.push_back({InlineItem::Kind::Text, static_cast<const Text*>(child)->data(), &nodeStyle});
                continue;
            }
            if (!child->isElement()) continue;
            const Element& el = *child->asElement();
            const ComputedStyle* st = storeStyle(resolver_.compute(el, &nodeStyle));
            if (st->display == Display::None) continue;
            if (el.tagName() == "br") { run.push_back({InlineItem::Kind::Break, "", st}); continue; }
            if (el.tagName() == "img") {
                std::string alt = el.attribute("alt");
                if (!alt.empty()) run.push_back({InlineItem::Kind::Text, "[" + alt + "]", st});
                continue;
            }
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

    float layoutInline(LayoutBox& b) {
        const ComputedStyle& bs = *b.style;
        const float avail = b.contentWidth;
        const float strut = bs.usedLineHeight();
        LineState cur;
        float y = 0;
        bool pendingSpace = false;
        const ComputedStyle* pendingStyle = nullptr;

        auto finishLine = [&](bool force) {
            if (cur.line.fragments.empty() && !force) { cur = LineState(); return; }
            float h = strut;
            for (const auto& f : cur.line.fragments) h = std::max(h, f.style->usedLineHeight());
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

        auto appendFragment = [&](const std::string& text, float w, const ComputedStyle* st) {
            auto& frags = cur.line.fragments;
            if (!frags.empty() && frags.back().style == st && std::fabs(frags.back().x + frags.back().width - cur.x) < 0.01f) {
                frags.back().text += text;
                frags.back().width += w;
            } else {
                LineFragment f;
                f.x = cur.x; f.width = w; f.text = text; f.style = st;
                frags.push_back(std::move(f));
            }
            cur.x += w;
        };

        auto addWord = [&](const std::string& word, const ComputedStyle* st) {
            if (word.empty()) return;
            float fs = st->fontSize;
            float w = measureText(word, fs);
            bool hasContent = !cur.line.fragments.empty();
            float spaceW = (pendingSpace && hasContent) ? glyphAdvance(pendingStyle->fontSize) : 0;
            if (!st->noWrap && hasContent && cur.x + spaceW + w > avail + 0.01f) {
                finishLine(false);
                hasContent = false;
                spaceW = 0;
            }
            if (spaceW > 0) appendFragment(" ", spaceW, pendingStyle);
            pendingSpace = false;
            if (!st->noWrap && w > avail && avail > 0) {
                // Break an over-long word between characters.
                std::string piece;
                float pw = 0;
                size_t i = 0;
                float adv = glyphAdvance(fs);
                while (i < word.size()) {
                    size_t before = i;
                    decodeUtf8(word, i);
                    if (pw + adv > avail - cur.x + 0.01f && !piece.empty()) {
                        appendFragment(piece, pw, st);
                        finishLine(false);
                        piece.clear(); pw = 0;
                    }
                    piece += word.substr(before, i - before);
                    pw += adv;
                }
                if (!piece.empty()) appendFragment(piece, pw, st);
                return;
            }
            appendFragment(word, w, st);
        };

        for (const auto& item : b.inlineItems) {
            if (item.kind == InlineItem::Kind::Break) {
                finishLine(true);
                continue;
            }
            const ComputedStyle* st = item.style;
            if (st->preserveWhitespace) {
                std::string seg;
                for (char c : item.text) {
                    if (c == '\r') continue;
                    if (c == '\n') { addWord(seg, st); seg.clear(); finishLine(true); }
                    else if (c == '\t') seg += "    ";
                    else seg.push_back(c);
                }
                addWord(seg, st);
                continue;
            }
            std::string word;
            for (char c : item.text) {
                if (isAsciiSpace(c)) {
                    if (!word.empty()) { addWord(word, st); word.clear(); }
                    if (!cur.line.fragments.empty()) { pendingSpace = true; pendingStyle = st; }
                } else word.push_back(c);
            }
            if (!word.empty()) addWord(word, st);
        }
        finishLine(false);
        return y;
    }
};

} // namespace

std::unique_ptr<LayoutTree> buildLayout(const Document& doc, const StyleResolver& resolver, const ViewportInfo& viewport) {
    auto tree = std::make_unique<LayoutTree>();
    const Element* html = doc.documentElement();
    if (!html) return tree;
    Builder builder(resolver, *tree);
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

} // namespace HB
