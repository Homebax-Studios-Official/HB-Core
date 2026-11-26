// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "html/tokenizer.hpp"
#include "base/strings.hpp"
#include <cctype>
#include <cstdlib>

namespace HB {

namespace {

struct NamedRef { const char* name; char32_t cp; };
const NamedRef kNamed[] = {
    {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''}, {"nbsp", 0xA0},
    {"copy", 0xA9}, {"reg", 0xAE}, {"trade", 0x2122}, {"hellip", 0x2026}, {"mdash", 0x2014},
    {"ndash", 0x2013}, {"lsquo", 0x2018}, {"rsquo", 0x2019}, {"ldquo", 0x201C}, {"rdquo", 0x201D},
    {"bull", 0x2022}, {"middot", 0xB7}, {"laquo", 0xAB}, {"raquo", 0xBB}, {"euro", 0x20AC},
    {"pound", 0xA3}, {"yen", 0xA5}, {"cent", 0xA2}, {"sect", 0xA7}, {"deg", 0xB0},
    {"times", 0xD7}, {"divide", 0xF7}, {"larr", 0x2190}, {"rarr", 0x2192}, {"uarr", 0x2191},
    {"darr", 0x2193}, {"hearts", 0x2665}, {"para", 0xB6}, {"plusmn", 0xB1},
};

bool isVoidWhitespace(char c) { return isAsciiSpace(c); }

bool isRawTextTag(const std::string& n) { return n == "script" || n == "style"; }
bool isRcdataTag(const std::string& n) { return n == "title" || n == "textarea"; }

} // namespace

std::string decodeHtmlEntities(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        if (s[i] != '&') { out.push_back(s[i++]); continue; }
        size_t j = i + 1;
        if (j < s.size() && s[j] == '#') {
            ++j;
            bool hex = j < s.size() && (s[j] == 'x' || s[j] == 'X');
            if (hex) ++j;
            size_t startDigits = j;
            unsigned long cp = 0;
            while (j < s.size() && (hex ? std::isxdigit(static_cast<unsigned char>(s[j])) : isAsciiDigit(s[j]))) {
                char c = s[j];
                unsigned d = isAsciiDigit(c) ? c - '0' : (std::tolower(static_cast<unsigned char>(c)) - 'a' + 10);
                cp = cp * (hex ? 16 : 10) + d;
                if (cp > 0x10FFFF) cp = 0x110000;
                ++j;
            }
            if (j == startDigits) { out.push_back('&'); ++i; continue; }
            if (j < s.size() && s[j] == ';') ++j;
            if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
            appendUtf8(out, static_cast<char32_t>(cp));
            i = j;
            continue;
        }
        size_t nameStart = j;
        while (j < s.size() && isAsciiAlnum(s[j])) ++j;
        std::string_view name = s.substr(nameStart, j - nameStart);
        bool matched = false;
        if (j < s.size() && s[j] == ';' && !name.empty()) {
            for (const auto& r : kNamed) {
                if (name == r.name) { appendUtf8(out, r.cp); matched = true; break; }
            }
        }
        if (matched) i = j + 1;
        else { out.push_back('&'); ++i; }
    }
    return out;
}

void HtmlTokenizer::emitText(std::string text) { text_ += text; }

std::vector<HtmlToken> HtmlTokenizer::tokenize() {
    std::vector<HtmlToken> out;
    auto flushText = [&] {
        if (!text_.empty()) {
            HtmlToken t;
            t.type = HtmlTokenType::Character;
            t.data = std::move(text_);
            text_.clear();
            out.push_back(std::move(t));
        }
    };
    size_t textStart = 0;
    pos_ = 0;
    while (pos_ < in_.size()) {
        char c = in_[pos_];
        if (c != '<') { ++pos_; continue; }
        // Possible markup. Flush preceding text.
        if (pos_ > textStart) emitText(decodeHtmlEntities(in_.substr(textStart, pos_ - textStart)));
        size_t next = pos_ + 1;
        if (next >= in_.size()) { emitText("<"); ++pos_; textStart = pos_; continue; }
        char n = in_[next];
        if (n == '!' ) {
            flushText();
            if (in_.substr(next, 3) == "!--") readComment(out);
            else readDoctype(out);
        } else if (n == '/' || isAsciiAlpha(n)) {
            if (n == '/' && !(next + 1 < in_.size() && isAsciiAlpha(in_[next + 1]))) {
                // "</" not followed by a letter: bogus comment up to '>' (or text if "</>")
                flushText();
                size_t gt = in_.find('>', next);
                if (gt == std::string_view::npos) gt = in_.size();
                size_t bodyStart = next + 1;
                HtmlToken t;
                t.type = HtmlTokenType::Comment;
                if (gt > bodyStart) t.data = std::string(in_.substr(bodyStart, gt - bodyStart));
                out.push_back(std::move(t));
                pos_ = gt < in_.size() ? gt + 1 : gt;
            } else {
                flushText();
                readTag(out);
            }
        } else if (n == '?') {
            flushText();
            size_t gt = in_.find('>', next);
            if (gt == std::string_view::npos) gt = in_.size();
            HtmlToken t; t.type = HtmlTokenType::Comment; t.data = std::string(in_.substr(next, gt - next));
            out.push_back(std::move(t));
            pos_ = gt < in_.size() ? gt + 1 : gt;
        } else {
            emitText("<");
            ++pos_;
            textStart = pos_;
            continue;
        }
        textStart = pos_;
    }
    if (pos_ > textStart) emitText(decodeHtmlEntities(in_.substr(textStart, pos_ - textStart)));
    flushText();
    HtmlToken eof;
    eof.type = HtmlTokenType::EndOfFile;
    out.push_back(eof);
    return out;
}

void HtmlTokenizer::readComment(std::vector<HtmlToken>& out) {
    size_t start = pos_ + 4;  // after "<!--"
    size_t end = in_.find("-->", start);
    HtmlToken t;
    t.type = HtmlTokenType::Comment;
    if (end == std::string_view::npos) { t.data = start <= in_.size() ? std::string(in_.substr(start)) : ""; pos_ = in_.size(); }
    else { t.data = std::string(in_.substr(start, end - start)); pos_ = end + 3; }
    out.push_back(std::move(t));
}

void HtmlTokenizer::readDoctype(std::vector<HtmlToken>& out) {
    size_t gt = in_.find('>', pos_);
    std::string_view body = in_.substr(pos_ + 2, (gt == std::string_view::npos ? in_.size() : gt) - (pos_ + 2));
    pos_ = gt == std::string_view::npos ? in_.size() : gt + 1;
    if (body.size() >= 7 && equalsIgnoreCase(body.substr(0, 7), "doctype")) {
        auto rest = trim(body.substr(7));
        size_t e = 0;
        while (e < rest.size() && !isAsciiSpace(rest[e])) ++e;
        HtmlToken t;
        t.type = HtmlTokenType::Doctype;
        t.data = toLowerAscii(rest.substr(0, e));
        out.push_back(std::move(t));
    } else {
        HtmlToken t; t.type = HtmlTokenType::Comment; t.data = std::string(body);
        out.push_back(std::move(t));
    }
}

void HtmlTokenizer::readTag(std::vector<HtmlToken>& out) {
    size_t i = pos_ + 1;
    HtmlToken tok;
    tok.type = HtmlTokenType::StartTag;
    if (in_[i] == '/') { tok.type = HtmlTokenType::EndTag; ++i; }
    size_t nameStart = i;
    while (i < in_.size() && !isVoidWhitespace(in_[i]) && in_[i] != '/' && in_[i] != '>') ++i;
    tok.data = toLowerAscii(in_.substr(nameStart, i - nameStart));

    // Attributes
    while (i < in_.size()) {
        while (i < in_.size() && (isVoidWhitespace(in_[i]))) ++i;
        if (i >= in_.size()) break;
        if (in_[i] == '>') { ++i; break; }
        if (in_[i] == '/') {
            if (i + 1 < in_.size() && in_[i + 1] == '>') { tok.selfClosing = true; i += 2; break; }
            ++i; continue;
        }
        size_t ns = i;
        while (i < in_.size() && !isVoidWhitespace(in_[i]) && in_[i] != '=' && in_[i] != '>' &&
               !(in_[i] == '/' && i + 1 < in_.size() && in_[i + 1] == '>')) ++i;
        std::string name = toLowerAscii(in_.substr(ns, i - ns));
        std::string value;
        size_t save = i;
        while (i < in_.size() && isVoidWhitespace(in_[i])) ++i;
        if (i < in_.size() && in_[i] == '=') {
            ++i;
            while (i < in_.size() && isVoidWhitespace(in_[i])) ++i;
            if (i < in_.size() && (in_[i] == '"' || in_[i] == '\'')) {
                char q = in_[i++];
                size_t vs = i;
                while (i < in_.size() && in_[i] != q) ++i;
                value = decodeHtmlEntities(in_.substr(vs, i - vs));
                if (i < in_.size()) ++i;
            } else {
                size_t vs = i;
                while (i < in_.size() && !isVoidWhitespace(in_[i]) && in_[i] != '>') ++i;
                value = decodeHtmlEntities(in_.substr(vs, i - vs));
            }
        } else {
            i = save;
        }
        if (name.empty()) { if (i == ns) ++i; continue; }
        bool dup = false;
        for (const auto& a : tok.attrs) if (a.name == name) { dup = true; break; }
        if (!dup) tok.attrs.push_back({std::move(name), std::move(value)});
    }
    pos_ = i;
    bool startTag = tok.type == HtmlTokenType::StartTag;
    std::string tagName = tok.data;
    if (tok.type == HtmlTokenType::EndTag) { tok.attrs.clear(); tok.selfClosing = false; }
    out.push_back(std::move(tok));
    if (startTag && !out.back().selfClosing) {
        if (isRawTextTag(tagName)) readRawText(tagName, false, out);
        else if (isRcdataTag(tagName)) readRawText(tagName, true, out);
    }
}

void HtmlTokenizer::readRawText(const std::string& tag, bool decodeEntities, std::vector<HtmlToken>& out) {
    // Content runs until a matching "</tag" (case-insensitive) followed by whitespace, '/' or '>'.
    size_t i = pos_;
    size_t end = in_.size();
    while (i < in_.size()) {
        size_t lt = in_.find("</", i);
        if (lt == std::string_view::npos) break;
        size_t ns = lt + 2;
        if (ns + tag.size() <= in_.size() && equalsIgnoreCase(in_.substr(ns, tag.size()), tag)) {
            size_t after = ns + tag.size();
            if (after >= in_.size() || isVoidWhitespace(in_[after]) || in_[after] == '/' || in_[after] == '>') { end = lt; break; }
        }
        i = lt + 2;
    }
    if (end > pos_) {
        HtmlToken t;
        t.type = HtmlTokenType::Character;
        std::string_view raw = in_.substr(pos_, end - pos_);
        t.data = decodeEntities ? decodeHtmlEntities(raw) : std::string(raw);
        out.push_back(std::move(t));
    }
    pos_ = end;  // the end tag is tokenized by the main loop
}

} // namespace HB
