// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "css/stylesheet.hpp"
#include "base/strings.hpp"
#include <cstdlib>

namespace HB {

namespace {

std::string stripComments(std::string_view css) {
    std::string out;
    out.reserve(css.size());
    size_t i = 0;
    char quote = 0;
    while (i < css.size()) {
        char c = css[i];
        if (quote) {
            out.push_back(c);
            if (c == '\\' && i + 1 < css.size()) { out.push_back(css[i + 1]); i += 2; continue; }
            if (c == quote) quote = 0;
            ++i;
        } else if (c == '"' || c == '\'') { quote = c; out.push_back(c); ++i; }
        else if (c == '/' && i + 1 < css.size() && css[i + 1] == '*') {
            size_t e = css.find("*/", i + 2);
            i = e == std::string_view::npos ? css.size() : e + 2;
            out.push_back(' ');
        } else { out.push_back(c); ++i; }
    }
    return out;
}

// Given position of '{', returns the position of its matching '}' (or size if unbalanced).
size_t findBlockEnd(std::string_view s, size_t open) {
    int depth = 0;
    char quote = 0;
    for (size_t i = open; i < s.size(); ++i) {
        char c = s[i];
        if (quote) { if (c == '\\') ++i; else if (c == quote) quote = 0; continue; }
        if (c == '"' || c == '\'') quote = c;
        else if (c == '{') ++depth;
        else if (c == '}') { if (--depth == 0) return i; }
    }
    return s.size();
}

std::vector<std::string_view> splitTopLevel(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    int paren = 0, bracket = 0;
    char quote = 0;
    size_t start = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (quote) { if (c == '\\') ++i; else if (c == quote) quote = 0; continue; }
        if (c == '"' || c == '\'') quote = c;
        else if (c == '(') ++paren;
        else if (c == ')') { if (paren) --paren; }
        else if (c == '[') ++bracket;
        else if (c == ']') { if (bracket) --bracket; }
        else if (c == sep && !paren && !bracket) { out.push_back(s.substr(start, i - start)); start = i + 1; }
    }
    out.push_back(s.substr(start));
    return out;
}

bool isIdentChar(char c) { return isAsciiAlnum(c) || c == '-' || c == '_' || (static_cast<unsigned char>(c) >= 0x80); }

std::string readIdent(std::string_view s, size_t& i) {
    size_t b = i;
    while (i < s.size() && isIdentChar(s[i])) ++i;
    return std::string(s.substr(b, i - b));
}

std::string unquote(std::string_view v) {
    v = trim(v);
    if (v.size() >= 2 && (v.front() == '"' || v.front() == '\'') && v.back() == v.front()) v = v.substr(1, v.size() - 2);
    return std::string(v);
}

bool parseCompound(std::string_view s, CompoundSelector& c) {
    size_t i = 0;
    bool any = false;
    if (i < s.size() && s[i] == '*') { c.tag = "*"; ++i; any = true; }
    else if (i < s.size() && isIdentChar(s[i])) { c.tag = toLowerAscii(readIdent(s, i)); any = true; }
    while (i < s.size()) {
        char ch = s[i];
        if (ch == '#') { ++i; c.id = readIdent(s, i); any = true; }
        else if (ch == '.') { ++i; c.classes.push_back(readIdent(s, i)); any = true; }
        else if (ch == '[') {
            size_t e = s.find(']', i);
            if (e == std::string_view::npos) return false;
            std::string_view body = trim(s.substr(i + 1, e - i - 1));
            AttributeSelector a;
            size_t k = 0;
            while (k < body.size() && isIdentChar(body[k])) ++k;
            a.name = toLowerAscii(body.substr(0, k));
            std::string_view rest = trim(body.substr(k));
            if (!rest.empty()) {
                size_t opLen = 1;
                if (rest[0] == '=') a.op = '=';
                else if (rest.size() > 1 && rest[1] == '=' && std::string_view("~|^$*").find(rest[0]) != std::string_view::npos) { a.op = rest[0]; opLen = 2; }
                else return false;
                a.value = unquote(rest.substr(opLen));
            }
            c.attrs.push_back(std::move(a));
            i = e + 1;
            any = true;
        } else if (ch == ':') {
            ++i;
            bool pseudoElement = false;
            if (i < s.size() && s[i] == ':') { pseudoElement = true; ++i; }
            std::string name = toLowerAscii(readIdent(s, i));
            if (i < s.size() && s[i] == '(') {  // functional pseudo: skip balanced parens, unsupported
                int d = 0;
                while (i < s.size()) { if (s[i] == '(') ++d; else if (s[i] == ')' && --d == 0) { ++i; break; } ++i; }
                c.unsupported = true;
            } else if (pseudoElement) c.unsupported = true;
            else if (name == "first-child" || name == "last-child" || name == "only-child" || name == "root" || name == "empty") c.pseudoClasses.push_back(name);
            else c.unsupported = true;  // :hover, :focus, :visited ... need state we do not track yet
            any = true;
        } else return false;
    }
    return any;
}

} // namespace

bool MediaCondition::matches(float w) const {
    if (never) return false;
    if (minWidth >= 0 && w < minWidth) return false;
    if (maxWidth >= 0 && w > maxWidth) return false;
    return true;
}

std::vector<ComplexSelector> parseSelectorList(std::string_view text) {
    std::vector<ComplexSelector> out;
    for (std::string_view item : splitTopLevel(text, ',')) {
        item = trim(item);
        if (item.empty()) return {};  // an invalid selector invalidates the whole list (CSS rule)
        ComplexSelector sel;
        size_t i = 0;
        Combinator pending = Combinator::Descendant;
        bool havePart = false, ok = true;
        while (i < item.size() && ok) {
            bool sawSpace = false;
            while (i < item.size() && isAsciiSpace(item[i])) { ++i; sawSpace = true; }
            if (i >= item.size()) break;
            char c = item[i];
            if (c == '>' || c == '+' || c == '~') {
                if (!havePart) { ok = false; break; }
                pending = c == '>' ? Combinator::Child : c == '+' ? Combinator::NextSibling : Combinator::SubsequentSibling;
                ++i;
                while (i < item.size() && isAsciiSpace(item[i])) ++i;
                if (i >= item.size()) { ok = false; break; }
                sawSpace = false;
                // fallthrough to compound parsing with explicit combinator
                size_t b = i; int br = 0, pr = 0;
                while (i < item.size() && (br || pr || !(isAsciiSpace(item[i]) || item[i] == '>' || item[i] == '+' || item[i] == '~'))) {
                    if (item[i] == '[') ++br; else if (item[i] == ']') --br; else if (item[i] == '(') ++pr; else if (item[i] == ')') --pr;
                    ++i;
                }
                CompoundSelector comp;
                if (!parseCompound(item.substr(b, i - b), comp)) { ok = false; break; }
                sel.combinators.push_back(pending);
                sel.parts.push_back(std::move(comp));
                continue;
            }
            size_t b = i; int br = 0, pr = 0;
            while (i < item.size() && (br || pr || !(isAsciiSpace(item[i]) || item[i] == '>' || item[i] == '+' || item[i] == '~'))) {
                if (item[i] == '[') ++br; else if (item[i] == ']') --br; else if (item[i] == '(') ++pr; else if (item[i] == ')') --pr;
                ++i;
            }
            CompoundSelector comp;
            if (!parseCompound(item.substr(b, i - b), comp)) { ok = false; break; }
            if (havePart) sel.combinators.push_back(Combinator::Descendant);
            (void)sawSpace;
            sel.parts.push_back(std::move(comp));
            havePart = true;
        }
        if (!ok || sel.parts.empty()) return {};
        uint32_t a = 0, b = 0, cc = 0;
        for (const auto& p : sel.parts) {
            if (!p.id.empty()) ++a;
            b += static_cast<uint32_t>(p.classes.size() + p.attrs.size() + p.pseudoClasses.size());
            if (!p.tag.empty() && p.tag != "*") ++cc;
        }
        sel.specificity = (a << 16) | (b << 8) | cc;
        out.push_back(std::move(sel));
    }
    return out;
}

static bool compoundMatches(const CompoundSelector& c, const Element& el) {
    if (c.unsupported) return false;
    if (!c.tag.empty() && c.tag != "*" && c.tag != el.tagName()) return false;
    if (!c.id.empty() && el.attribute("id") != c.id) return false;
    for (const auto& cls : c.classes) if (!el.hasClass(cls)) return false;
    for (const auto& a : c.attrs) {
        const std::string* v = el.getAttribute(a.name);
        if (!v) return false;
        switch (a.op) {
            case 0: break;
            case '=': if (*v != a.value) return false; break;
            case '~': { bool f = false; for (auto& w : splitWhitespace(*v)) if (w == a.value) f = true; if (!f) return false; break; }
            case '|': if (!(*v == a.value || startsWith(*v, a.value + "-"))) return false; break;
            case '^': if (a.value.empty() || !startsWith(*v, a.value)) return false; break;
            case '$': if (a.value.empty() || !endsWith(*v, a.value)) return false; break;
            case '*': if (a.value.empty() || v->find(a.value) == std::string::npos) return false; break;
        }
    }
    for (const auto& p : c.pseudoClasses) {
        if (p == "first-child") { if (el.previousElementSibling()) return false; }
        else if (p == "last-child") { if (el.nextElementSibling()) return false; }
        else if (p == "only-child") { if (el.previousElementSibling() || el.nextElementSibling()) return false; }
        else if (p == "root") { if (!el.parent() || el.parent()->nodeType() != NodeType::Document) return false; }
        else if (p == "empty") {
            for (const auto& ch : el.children()) {
                if (ch->isElement()) return false;
                if (ch->isText() && !static_cast<const Text*>(ch.get())->data().empty()) return false;
            }
        }
    }
    return true;
}

static bool matchFrom(const ComplexSelector& sel, size_t idx, const Element& el) {
    if (!compoundMatches(sel.parts[idx], el)) return false;
    if (idx == 0) return true;
    Combinator comb = sel.combinators[idx - 1];
    switch (comb) {
        case Combinator::Child: {
            const Node* p = el.parent();
            return p && p->isElement() && matchFrom(sel, idx - 1, *p->asElement());
        }
        case Combinator::Descendant: {
            for (const Node* p = el.parent(); p; p = p->parent())
                if (p->isElement() && matchFrom(sel, idx - 1, *p->asElement())) return true;
            return false;
        }
        case Combinator::NextSibling: {
            const Element* s = el.previousElementSibling();
            return s && matchFrom(sel, idx - 1, *s);
        }
        case Combinator::SubsequentSibling: {
            for (const Element* s = el.previousElementSibling(); s; s = s->previousElementSibling())
                if (matchFrom(sel, idx - 1, *s)) return true;
            return false;
        }
    }
    return false;
}

bool selectorMatches(const ComplexSelector& sel, const Element& el) {
    if (sel.parts.empty()) return false;
    return matchFrom(sel, sel.parts.size() - 1, el);
}

std::vector<Declaration> parseDeclarations(std::string_view block) {
    std::vector<Declaration> out;
    std::string clean = stripComments(block);
    for (std::string_view item : splitTopLevel(clean, ';')) {
        item = trim(item);
        if (item.empty()) continue;
        size_t colon = item.find(':');
        if (colon == std::string_view::npos) continue;
        Declaration d;
        std::string_view name = trim(item.substr(0, colon));
        std::string_view value = trim(item.substr(colon + 1));
        if (name.empty() || value.empty()) continue;
        bool custom = startsWith(name, "--");
        d.property = custom ? std::string(name) : toLowerAscii(name);
        size_t bang = value.rfind('!');
        if (bang != std::string_view::npos && equalsIgnoreCase(trim(value.substr(bang + 1)), "important")) {
            d.important = true;
            value = trim(value.substr(0, bang));
        }
        if (value.empty()) continue;
        d.value = std::string(value);
        out.push_back(std::move(d));
    }
    return out;
}

static MediaCondition parseMedia(std::string_view prelude) {
    MediaCondition m;
    std::string p = toLowerAscii(trim(prelude));
    // Only a single query (no commas) of: [screen|all] [and (min-width: Npx)] [and (max-width: Npx)]
    if (p.find(',') != std::string::npos || p.find("not ") != std::string::npos || p.find(" or ") != std::string::npos) { m.never = true; return m; }
    size_t i = 0;
    while (i < p.size()) {
        if (isAsciiSpace(p[i])) { ++i; continue; }
        if (p[i] == '(') {
            size_t e = p.find(')', i);
            if (e == std::string::npos) { m.never = true; return m; }
            std::string feat = p.substr(i + 1, e - i - 1);
            size_t colon = feat.find(':');
            if (colon == std::string::npos) { m.never = true; return m; }
            std::string name = std::string(trim(feat.substr(0, colon)));
            std::string val = std::string(trim(feat.substr(colon + 1)));
            char* end = nullptr;
            float v = std::strtof(val.c_str(), &end);
            if (end == val.c_str() || std::string(end) != "px") { m.never = true; return m; }
            if (name == "min-width") m.minWidth = v;
            else if (name == "max-width") m.maxWidth = v;
            else { m.never = true; return m; }
            i = e + 1;
            continue;
        }
        size_t b = i;
        while (i < p.size() && !isAsciiSpace(p[i]) && p[i] != '(') ++i;
        std::string w = p.substr(b, i - b);
        if (w == "and" || w == "screen" || w == "all") continue;
        m.never = true;  // print, speech, orientation... are not evaluated
        return m;
    }
    return m;
}

static void parseRules(std::string_view css, const MediaCondition& media, Stylesheet& sheet) {
    size_t i = 0;
    while (i < css.size()) {
        while (i < css.size() && isAsciiSpace(css[i])) ++i;
        if (i >= css.size()) break;
        if (css[i] == '@') {
            size_t nameStart = i + 1, j = nameStart;
            while (j < css.size() && isIdentChar(css[j])) ++j;
            std::string name = toLowerAscii(css.substr(nameStart, j - nameStart));
            size_t brace = css.find('{', j);
            size_t semi = css.find(';', j);
            if (semi != std::string_view::npos && (brace == std::string_view::npos || semi < brace)) { i = semi + 1; continue; }
            if (brace == std::string_view::npos) break;
            size_t end = findBlockEnd(css, brace);
            if (name == "media") {
                MediaCondition inner = parseMedia(css.substr(j, brace - j));
                MediaCondition combined = media;
                if (inner.never) combined.never = true;
                if (inner.minWidth >= 0) combined.minWidth = std::max(combined.minWidth, inner.minWidth);
                if (inner.maxWidth >= 0) combined.maxWidth = combined.maxWidth < 0 ? inner.maxWidth : std::min(combined.maxWidth, inner.maxWidth);
                parseRules(css.substr(brace + 1, end - brace - 1), combined, sheet);
            }
            i = end < css.size() ? end + 1 : end;
            continue;
        }
        size_t brace = css.find('{', i);
        if (brace == std::string_view::npos) break;
        size_t end = findBlockEnd(css, brace);
        std::string_view prelude = css.substr(i, brace - i);
        std::string_view body = css.substr(brace + 1, end - brace - 1);
        auto sels = parseSelectorList(prelude);
        if (!sels.empty()) {
            StyleRule r;
            r.selectors = std::move(sels);
            r.declarations = parseDeclarations(body);
            r.media = media;
            r.order = static_cast<uint32_t>(sheet.rules.size());
            sheet.rules.push_back(std::move(r));
        }
        i = end < css.size() ? end + 1 : end;
    }
}

Stylesheet parseStylesheet(std::string_view css) {
    Stylesheet sheet;
    std::string clean = stripComments(css);
    parseRules(clean, MediaCondition{}, sheet);
    return sheet;
}

} // namespace HB
