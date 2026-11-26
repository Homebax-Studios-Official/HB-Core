// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "css/style.hpp"
#include "base/strings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace HB {

namespace {

constexpr int kTop = 0, kRight = 1, kBottom = 2, kLeft = 3;

struct Matched {
    int priority;       // 0 UA normal, 1 author normal, 2 author important, 3 UA important
    uint32_t specificity;
    uint32_t order;     // global source order
    const Declaration* decl;
};

std::vector<std::string> splitValueTokens(const std::string& v) {
    std::vector<std::string> out;
    std::string cur;
    int paren = 0;
    for (char c : v) {
        if (c == '(') ++paren;
        if (c == ')' && paren) --paren;
        if (isAsciiSpace(c) && !paren) { if (!cur.empty()) { out.push_back(cur); cur.clear(); } }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Replaces var(--name[, fallback]) using customProperties. Depth-limited to avoid cycles.
std::string substituteVars(const std::string& value, const std::map<std::string, std::string>& props, int depth = 0) {
    if (depth > 8) return value;
    std::string out;
    size_t i = 0;
    while (i < value.size()) {
        size_t v = value.find("var(", i);
        if (v == std::string::npos) { out += value.substr(i); break; }
        out += value.substr(i, v - i);
        size_t j = v + 4;
        int paren = 1;
        size_t k = j;
        while (k < value.size() && paren) {
            if (value[k] == '(') ++paren;
            else if (value[k] == ')') --paren;
            ++k;
        }
        if (paren) { out += value.substr(v); break; }
        std::string inner = value.substr(j, k - 1 - j);
        std::string name, fallback;
        size_t comma = inner.find(',');
        if (comma == std::string::npos) name = std::string(trim(inner));
        else { name = std::string(trim(inner.substr(0, comma))); fallback = std::string(trim(inner.substr(comma + 1))); }
        auto it = props.find(name);
        std::string replacement = it != props.end() ? it->second : fallback;
        out += substituteVars(replacement, props, depth + 1);
        i = k;
    }
    return out;
}


float fontSizeKeyword(const std::string& v, bool& ok) {
    ok = true;
    if (v == "xx-small") return 9;
    if (v == "x-small") return 10;
    if (v == "small") return 13;
    if (v == "medium") return 16;
    if (v == "large") return 18;
    if (v == "x-large") return 24;
    if (v == "xx-large") return 32;
    ok = false;
    return 0;
}

struct Ctx {
    ComputedStyle& s;
    const ComputedStyle* parent;
    float rootFontSize;
    const ViewportInfo& vp;
};

void setLengthSides(Length* arr, const std::vector<std::string>& toks, Ctx& c, bool allowAuto) {
    if (toks.empty() || toks.size() > 4) return;
    Length vals[4];
    for (size_t i = 0; i < toks.size(); ++i) {
        Length l;
        if (!parseLengthValue(toks[i], c.s.fontSize, c.rootFontSize, c.vp, l)) return;
        if (l.isAuto() && !allowAuto) return;
        vals[i] = l;
    }
    Length t, r, b, l;
    switch (toks.size()) {
        case 1: t = r = b = l = vals[0]; break;
        case 2: t = b = vals[0]; r = l = vals[1]; break;
        case 3: t = vals[0]; r = l = vals[1]; b = vals[2]; break;
        default: t = vals[0]; r = vals[1]; b = vals[2]; l = vals[3]; break;
    }
    arr[kTop] = t; arr[kRight] = r; arr[kBottom] = b; arr[kLeft] = l;
}

bool parseBorderWidth(const std::string& tok, Ctx& c, float& out) {
    if (tok == "thin") { out = 1; return true; }
    if (tok == "medium") { out = 3; return true; }
    if (tok == "thick") { out = 5; return true; }
    Length l;
    if (!parseLengthValue(tok, c.s.fontSize, c.rootFontSize, c.vp, l) || l.kind != Length::Kind::Px || l.value < 0) return false;
    out = l.value;
    return true;
}

bool isBorderStyle(const std::string& t) {
    static const char* kStyles[] = {"none", "hidden", "solid", "dotted", "dashed", "double", "groove", "ridge", "inset", "outset"};
    for (auto s : kStyles) if (t == s) return true;
    return false;
}

void applyBorderParts(int side, const std::string& value, Ctx& c) {
    // side: 0..3, or -1 for all
    float width = 3;
    bool haveStyle = false, visible = false;
    Color color;
    bool haveColor = false;
    for (const auto& tok : splitValueTokens(value)) {
        std::string t = toLowerAscii(tok);
        float w;
        if (isBorderStyle(t)) { haveStyle = true; visible = !(t == "none" || t == "hidden"); }
        else if (t == "currentcolor") { haveColor = false; }
        else if (auto col = parseColor(t)) { color = *col; haveColor = true; }
        else if (parseBorderWidth(t, c, w)) width = w;
        else return;
    }
    for (int i = 0; i < 4; ++i) {
        if (side >= 0 && i != side) continue;
        c.s.borderWidth[i] = width;
        c.s.borderVisible[i] = haveStyle && visible;
        c.s.borderColorSet[i] = haveColor;
        if (haveColor) c.s.borderColor[i] = color;
    }
}

void applyProperty(const std::string& name, const std::string& rawValue, Ctx& c) {
    ComputedStyle& s = c.s;
    std::string value(trim(rawValue));
    std::string lower = toLowerAscii(value);
    auto len = [&](Length& out) {
        Length l;
        if (parseLengthValue(value, s.fontSize, c.rootFontSize, c.vp, l)) out = l;
    };
    const ComputedStyle* p = c.parent;

    if (lower == "inherit" && p) {
        // Minimal support for `inherit` on the properties we compute.
        if (name == "color") s.color = p->color;
        else if (name == "background-color") s.backgroundColor = p->backgroundColor;
        else if (name == "font-weight") s.bold = p->bold;
        else if (name == "font-style") s.italic = p->italic;
        else if (name == "text-align") s.textAlign = p->textAlign;
        else if (name == "text-decoration") { s.underline = p->underline; s.lineThrough = p->lineThrough; }
        else if (name == "display") s.display = p->display;
        return;
    }

    if (name == "display") {
        if (lower == "none") s.display = Display::None;
        else if (lower == "inline" || lower == "inline-block" || lower == "inline-flex" || lower == "inline-grid" || lower == "inline-table") s.display = Display::Inline;
        else if (lower == "list-item") s.display = Display::ListItem;
        else if (lower == "contents") s.display = Display::Inline;
        else s.display = Display::Block;  // block, flex, grid, table*: approximated as block in 0.1.0
    } else if (name == "color") {
        if (lower == "currentcolor") return;
        if (auto col = parseColor(lower)) s.color = *col;
    } else if (name == "background-color") {
        if (lower == "currentcolor") s.backgroundColor = s.color;
        else if (auto col = parseColor(lower)) s.backgroundColor = *col;
    } else if (name == "background") {
        // Only a plain color layer is supported; images and gradients are ignored.
        bool found = false;
        for (const auto& tok : splitValueTokens(lower)) {
            if (auto col = parseColor(tok)) { s.backgroundColor = *col; found = true; }
        }
        if (!found && lower == "none") s.backgroundColor = Color::transparent();
    } else if (name == "font-size") {
        bool kw;
        float k = fontSizeKeyword(lower, kw);
        float parentSize = p ? p->fontSize : 16.0f;
        if (kw) s.fontSize = k;
        else if (lower == "larger") s.fontSize = parentSize * 1.2f;
        else if (lower == "smaller") s.fontSize = parentSize / 1.2f;
        else {
            Length l;
            // em/% on font-size are relative to the parent's font size
            if (parseLengthValue(value, parentSize, c.rootFontSize, c.vp, l) && l.kind != Length::Kind::Auto) {
                float v = l.kind == Length::Kind::Percent ? parentSize * l.value / 100.0f : l.value;
                if (v >= 0) s.fontSize = v;
            }
        }
    } else if (name == "font-weight") {
        if (lower == "bold" || lower == "bolder") s.bold = true;
        else if (lower == "normal" || lower == "lighter") s.bold = false;
        else { char* e; long n = std::strtol(lower.c_str(), &e, 10); if (e != lower.c_str() && *e == 0) s.bold = n >= 600; }
    } else if (name == "font-style") {
        s.italic = lower == "italic" || lower == "oblique";
    } else if (name == "text-decoration" || name == "text-decoration-line") {
        s.underline = lower.find("underline") != std::string::npos;
        s.lineThrough = lower.find("line-through") != std::string::npos;
    } else if (name == "text-align") {
        if (lower == "center") s.textAlign = TextAlign::Center;
        else if (lower == "right" || lower == "end") s.textAlign = TextAlign::Right;
        else if (lower == "left" || lower == "start") s.textAlign = TextAlign::Left;
    } else if (name == "line-height") {
        if (lower == "normal") { s.lineHeightMode = ComputedStyle::LineHeightMode::Normal; }
        else {
            char* e; float n = std::strtof(lower.c_str(), &e);
            if (e != lower.c_str() && *e == 0 && n >= 0) { s.lineHeightMode = ComputedStyle::LineHeightMode::Number; s.lineHeightValue = n; }
            else {
                Length l;
                if (parseLengthValue(value, s.fontSize, c.rootFontSize, c.vp, l) && !l.isAuto()) {
                    s.lineHeightMode = ComputedStyle::LineHeightMode::Px;
                    s.lineHeightValue = l.kind == Length::Kind::Percent ? s.fontSize * l.value / 100.0f : l.value;
                }
            }
        }
    } else if (name == "white-space") {
        s.preserveWhitespace = lower == "pre" || lower == "pre-wrap" || lower == "pre-line" || lower == "break-spaces";
        s.noWrap = lower == "nowrap" || lower == "pre";
    } else if (name == "list-style-type" || name == "list-style") {
        if (lower.find("none") != std::string::npos) s.listStyle = ListStyle::None;
        else if (lower.find("decimal") != std::string::npos) s.listStyle = ListStyle::Decimal;
        else if (lower.find("circle") != std::string::npos) s.listStyle = ListStyle::Circle;
        else if (lower.find("square") != std::string::npos) s.listStyle = ListStyle::Square;
        else if (lower.find("disc") != std::string::npos) s.listStyle = ListStyle::Disc;
    } else if (name == "visibility") {
        s.visible = !(lower == "hidden" || lower == "collapse");
    } else if (name == "overflow" || name == "overflow-x" || name == "overflow-y") {
        s.overflowHidden = lower == "hidden" || lower == "scroll" || lower == "auto" || lower == "clip";
    } else if (name == "opacity") {
        char* e; float o = std::strtof(lower.c_str(), &e);
        if (e != lower.c_str()) s.opacity = std::clamp(o, 0.0f, 1.0f);
    } else if (name == "margin") setLengthSides(s.margin, splitValueTokens(value), c, true);
    else if (name == "padding") setLengthSides(s.padding, splitValueTokens(value), c, false);
    else if (name == "margin-top") len(s.margin[kTop]);
    else if (name == "margin-right") len(s.margin[kRight]);
    else if (name == "margin-bottom") len(s.margin[kBottom]);
    else if (name == "margin-left") len(s.margin[kLeft]);
    else if (name == "padding-top") len(s.padding[kTop]);
    else if (name == "padding-right") len(s.padding[kRight]);
    else if (name == "padding-bottom") len(s.padding[kBottom]);
    else if (name == "padding-left") len(s.padding[kLeft]);
    else if (name == "width") len(s.width);
    else if (name == "height") len(s.height);
    else if (name == "min-width") len(s.minWidth);
    else if (name == "min-height") len(s.minHeight);
    else if (name == "max-width") { if (lower == "none") s.maxWidth = Length::autoLength(); else len(s.maxWidth); }
    else if (name == "border") applyBorderParts(-1, lower, c);
    else if (name == "border-top") applyBorderParts(kTop, lower, c);
    else if (name == "border-right") applyBorderParts(kRight, lower, c);
    else if (name == "border-bottom") applyBorderParts(kBottom, lower, c);
    else if (name == "border-left") applyBorderParts(kLeft, lower, c);
    else if (name == "border-width") {
        auto toks = splitValueTokens(lower);
        if (toks.empty() || toks.size() > 4) return;
        float v[4]; 
        for (size_t i = 0; i < toks.size(); ++i) if (!parseBorderWidth(toks[i], c, v[i])) return;
        float t, r, b, l;
        switch (toks.size()) { case 1: t = r = b = l = v[0]; break; case 2: t = b = v[0]; r = l = v[1]; break; case 3: t = v[0]; r = l = v[1]; b = v[2]; break; default: t = v[0]; r = v[1]; b = v[2]; l = v[3]; }
        s.borderWidth[kTop] = t; s.borderWidth[kRight] = r; s.borderWidth[kBottom] = b; s.borderWidth[kLeft] = l;
    } else if (name == "border-style") {
        auto toks = splitValueTokens(lower);
        if (toks.empty() || toks.size() > 4) return;
        bool vis[4] = {};
        for (size_t i = 0; i < toks.size(); ++i) { if (!isBorderStyle(toks[i])) return; vis[i] = !(toks[i] == "none" || toks[i] == "hidden"); }
        bool t, r, b, l;
        switch (toks.size()) { case 1: t = r = b = l = vis[0]; break; case 2: t = b = vis[0]; r = l = vis[1]; break; case 3: t = vis[0]; r = l = vis[1]; b = vis[2]; break; default: t = vis[0]; r = vis[1]; b = vis[2]; l = vis[3]; }
        s.borderVisible[kTop] = t; s.borderVisible[kRight] = r; s.borderVisible[kBottom] = b; s.borderVisible[kLeft] = l;
    } else if (name == "border-color") {
        auto toks = splitValueTokens(lower);
        if (toks.empty() || toks.size() > 4) return;
        Color v[4];
        for (size_t i = 0; i < toks.size(); ++i) { auto col = parseColor(toks[i]); if (!col) return; v[i] = *col; }
        Color cols[4];
        switch (toks.size()) { case 1: cols[0] = cols[1] = cols[2] = cols[3] = v[0]; break; case 2: cols[0] = cols[2] = v[0]; cols[1] = cols[3] = v[1]; break; case 3: cols[0] = v[0]; cols[1] = cols[3] = v[1]; cols[2] = v[2]; break; default: for (int i = 0; i < 4; ++i) cols[i] = v[i]; }
        for (int i = 0; i < 4; ++i) { s.borderColor[i] = cols[i]; s.borderColorSet[i] = true; }
    } else if (name == "border-top-width") { float w; if (parseBorderWidth(lower, c, w)) s.borderWidth[kTop] = w; }
    else if (name == "border-right-width") { float w; if (parseBorderWidth(lower, c, w)) s.borderWidth[kRight] = w; }
    else if (name == "border-bottom-width") { float w; if (parseBorderWidth(lower, c, w)) s.borderWidth[kBottom] = w; }
    else if (name == "border-left-width") { float w; if (parseBorderWidth(lower, c, w)) s.borderWidth[kLeft] = w; }
    else if (name == "border-top-style") { if (isBorderStyle(lower)) s.borderVisible[kTop] = !(lower == "none" || lower == "hidden"); }
    else if (name == "border-right-style") { if (isBorderStyle(lower)) s.borderVisible[kRight] = !(lower == "none" || lower == "hidden"); }
    else if (name == "border-bottom-style") { if (isBorderStyle(lower)) s.borderVisible[kBottom] = !(lower == "none" || lower == "hidden"); }
    else if (name == "border-left-style") { if (isBorderStyle(lower)) s.borderVisible[kLeft] = !(lower == "none" || lower == "hidden"); }
    else if (name == "border-top-color") { if (auto col = parseColor(lower)) { s.borderColor[kTop] = *col; s.borderColorSet[kTop] = true; } }
    else if (name == "border-right-color") { if (auto col = parseColor(lower)) { s.borderColor[kRight] = *col; s.borderColorSet[kRight] = true; } }
    else if (name == "border-bottom-color") { if (auto col = parseColor(lower)) { s.borderColor[kBottom] = *col; s.borderColorSet[kBottom] = true; } }
    else if (name == "border-left-color") { if (auto col = parseColor(lower)) { s.borderColor[kLeft] = *col; s.borderColorSet[kLeft] = true; } }
    // Unknown properties are ignored (and reported as unsupported in SUPPORT_MATRIX.md).
}

} // namespace

bool parseLengthValue(const std::string& text, float fontSize, float rootFontSize, const ViewportInfo& vp, Length& out) {
    std::string t = toLowerAscii(trim(text));
    if (t.empty()) return false;
    if (t == "auto") { out = Length::autoLength(); return true; }
    char* end = nullptr;
    float v = std::strtof(t.c_str(), &end);
    if (end == t.c_str()) return false;
    std::string unit(end);
    if (unit.empty()) { if (v == 0) { out = Length::px(0); return true; } return false; }
    if (unit == "px") out = Length::px(v);
    else if (unit == "%") out = Length::percent(v);
    else if (unit == "em") out = Length::px(v * fontSize);
    else if (unit == "rem") out = Length::px(v * rootFontSize);
    else if (unit == "pt") out = Length::px(v * 96.0f / 72.0f);
    else if (unit == "pc") out = Length::px(v * 16.0f);
    else if (unit == "in") out = Length::px(v * 96.0f);
    else if (unit == "cm") out = Length::px(v * 96.0f / 2.54f);
    else if (unit == "mm") out = Length::px(v * 96.0f / 25.4f);
    else if (unit == "vw") out = Length::px(v * vp.width / 100.0f);
    else if (unit == "vh") out = Length::px(v * vp.height / 100.0f);
    else return false;  // calc(), clamp(), ch, ex, ... not supported yet
    return true;
}

ComputedStyle StyleResolver::compute(const Element& el, const ComputedStyle* parent) const {
    ComputedStyle s;
    if (parent) {
        s.color = parent->color;
        s.fontSize = parent->fontSize;
        s.bold = parent->bold;
        s.italic = parent->italic;
        s.underline = parent->underline;
        s.lineThrough = parent->lineThrough;
        s.textAlign = parent->textAlign;
        s.lineHeightMode = parent->lineHeightMode;
        s.lineHeightValue = parent->lineHeightValue;
        s.preserveWhitespace = parent->preserveWhitespace;
        s.noWrap = parent->noWrap;
        s.listStyle = parent->listStyle;
        s.visible = parent->visible;
        s.customProperties = parent->customProperties;
    }

    // Collect matching declarations.
    std::vector<Matched> matched;
    std::vector<Declaration> inlineDecls;
    uint32_t order = 0;
    auto collect = [&](const Stylesheet* sheet, bool isUA) {
        if (!sheet) return;
        for (const auto& rule : sheet->rules) {
            if (!rule.media.matches(viewport_.width)) continue;
            uint32_t best = 0;
            bool any = false;
            for (const auto& sel : rule.selectors) {
                if (selectorMatches(sel, el)) { any = true; best = std::max(best, sel.specificity); }
            }
            if (!any) continue;
            for (const auto& d : rule.declarations) {
                int pr = d.important ? (isUA ? 3 : 2) : (isUA ? 0 : 1);
                matched.push_back({pr, best, order++, &d});
            }
        }
    };
    collect(ua_, true);
    for (const Stylesheet* sh : author_) collect(sh, false);
    if (const std::string* styleAttr = el.getAttribute("style")) {
        inlineDecls = parseDeclarations(*styleAttr);
        for (const auto& d : inlineDecls) {
            matched.push_back({d.important ? 2 : 1, 0xFFFFFFFFu, order++, &d});
        }
    }
    std::stable_sort(matched.begin(), matched.end(), [](const Matched& a, const Matched& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.specificity != b.specificity) return a.specificity < b.specificity;
        return a.order < b.order;
    });

    // 1. custom properties (so var() can resolve)
    for (const auto& m : matched) {
        if (startsWith(m.decl->property, "--")) s.customProperties[m.decl->property] = m.decl->value;
    }
    // 2. font-size first (em units of other properties depend on it)
    float rootFont = 16.0f;
    Ctx ctx{s, parent, rootFont, viewport_};
    for (const auto& m : matched) {
        if (m.decl->property == "font-size") {
            applyProperty("font-size", substituteVars(m.decl->value, s.customProperties), ctx);
        }
    }
    // 3. everything else in cascade order
    for (const auto& m : matched) {
        const std::string& prop = m.decl->property;
        if (prop == "font-size" || startsWith(prop, "--")) continue;
        applyProperty(prop, substituteVars(m.decl->value, s.customProperties), ctx);
    }
    return s;
}

} // namespace HB
