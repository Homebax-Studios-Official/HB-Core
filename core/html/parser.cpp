// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "html/parser.hpp"
#include "html/tokenizer.hpp"
#include "base/strings.hpp"
#include <algorithm>
#include <array>

namespace HB {

namespace {

template <size_t N>
bool inList(const std::array<const char*, N>& list, std::string_view s) {
    for (const char* x : list) if (s == x) return true;
    return false;
}

const std::array<const char*, 14> kVoid = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"};
const std::array<const char*, 8> kHeadTags = {"title", "meta", "link", "style", "script", "base", "noscript", "template"};
// Start tags that implicitly close an open <p>.
const std::array<const char*, 32> kClosesP = {"address", "article", "aside", "blockquote", "details", "div", "dl", "fieldset", "figcaption", "figure", "footer", "form", "h1", "h2", "h3", "h4", "h5", "h6", "header", "hr", "main", "nav", "ol", "p", "pre", "section", "table", "ul", "menu", "dir", "center", "li"};
// Elements that stop the "in scope" search for end tags.
const std::array<const char*, 9> kScopeBarriers = {"html", "table", "td", "th", "caption", "marquee", "object", "template", "body"};

class TreeBuilder {
public:
    TreeBuilder() : doc_(std::make_unique<Document>()) {}

    std::unique_ptr<Document> build(std::string_view html) {
        HtmlTokenizer tz(html);
        for (auto& tok : tz.tokenize()) process(tok);
        ensureHtml();
        ensureHead();
        ensureBody();
        return std::move(doc_);
    }

private:
    Node* current() { return stack_.empty() ? static_cast<Node*>(doc_.get()) : stack_.back(); }

    Element* insertElement(const HtmlToken& tok) {
        auto el = std::make_unique<Element>(tok.data);
        for (const auto& a : tok.attrs) el->setAttribute(a.name, a.value);
        Element* raw = static_cast<Element*>(current()->appendChild(std::move(el)));
        stack_.push_back(raw);
        return raw;
    }

    Element* insertSimple(const std::string& tag) {
        HtmlToken t; t.type = HtmlTokenType::StartTag; t.data = tag;
        return insertElement(t);
    }

    void ensureHtml() {
        if (!html_) html_ = insertSimple("html");
    }
    void ensureHead() {
        ensureHtml();
        if (!head_) {
            // head belongs directly below html
            while (!stack_.empty() && stack_.back() != html_) stack_.pop_back();
            head_ = insertSimple("head");
            inHead_ = true;
        }
    }
    void closeHead() {
        if (inHead_) {
            while (!stack_.empty() && stack_.back() != html_) stack_.pop_back();
            inHead_ = false;
        }
    }
    void ensureBody() {
        ensureHead();
        closeHead();
        if (!body_) {
            while (!stack_.empty() && stack_.back() != html_) stack_.pop_back();
            body_ = insertSimple("body");
        }
    }

    bool inScope(const std::string& tag) const {
        for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
            if ((*it)->tagName() == tag) return true;
            if (inList(kScopeBarriers, (*it)->tagName())) return false;
        }
        return false;
    }
    bool inButtonScopeP() const { return inScope("p"); }

    void popUntil(const std::string& tag) {
        while (!stack_.empty()) {
            std::string n = stack_.back()->tagName();
            stack_.pop_back();
            if (n == tag) break;
        }
    }

    void closeImpliedForStart(const std::string& tag) {
        if (inList(kClosesP, tag) && inButtonScopeP()) popUntil("p");
        if (tag == "li") {
            for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
                const std::string& n = (*it)->tagName();
                if (n == "li") { popUntil("li"); break; }
                if (n == "ul" || n == "ol" || n == "div" || n == "table" || n == "body") break;
            }
        } else if (tag == "dt" || tag == "dd") {
            for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
                const std::string& n = (*it)->tagName();
                if (n == "dt" || n == "dd") { popUntil(n); break; }
                if (n == "dl" || n == "div" || n == "body") break;
            }
        } else if (tag == "tr") {
            if (inScope("tr")) popUntil("tr");
        } else if (tag == "td" || tag == "th") {
            if (inScope("td")) popUntil("td");
            else if (inScope("th")) popUntil("th");
        } else if (tag == "option") {
            if (!stack_.empty() && stack_.back()->tagName() == "option") stack_.pop_back();
        } else if (tag.size() == 2 && tag[0] == 'h' && tag[1] >= '1' && tag[1] <= '6') {
            if (!stack_.empty()) {
                const std::string& n = stack_.back()->tagName();
                if (n.size() == 2 && n[0] == 'h' && n[1] >= '1' && n[1] <= '6') stack_.pop_back();
            }
        }
    }

    void appendText(const std::string& text) {
        std::string_view sv(text);
        Node* cur = current();
        if (Node* last = cur->lastChild(); last && last->isText()) {
            static_cast<Text*>(last)->append(sv);
        } else {
            cur->appendChild(std::make_unique<Text>(text));
        }
    }

    void process(const HtmlToken& tok) {
        switch (tok.type) {
            case HtmlTokenType::Doctype:
                if (!html_ && !doc_->hasDoctype) {
                    doc_->appendChild(std::make_unique<DocumentType>(tok.data));
                    doc_->hasDoctype = true;
                    doc_->quirksMode = tok.data != "html";
                }
                break;
            case HtmlTokenType::Comment:
                current()->appendChild(std::make_unique<Comment>(tok.data));
                break;
            case HtmlTokenType::Character: onText(tok.data); break;
            case HtmlTokenType::StartTag: onStart(tok); break;
            case HtmlTokenType::EndTag: onEnd(tok); break;
            case HtmlTokenType::EndOfFile: break;
        }
    }

    void onText(const std::string& text) {
        bool allSpace = std::all_of(text.begin(), text.end(), [](char c) { return isAsciiSpace(c); });
        if (!body_) {
            if (inHead_ && !stack_.empty() && stack_.back() != head_ && stack_.back() != html_) {
                appendText(text);  // inside title/style/script
                return;
            }
            if (allSpace) return;  // whitespace before <body> is dropped
            ensureBody();
        }
        appendText(text);
    }

    void onStart(const HtmlToken& tok) {
        const std::string& tag = tok.data;
        if (tag == "html") {
            if (!html_) { html_ = insertElement(tok); }
            else for (const auto& a : tok.attrs) if (!html_->hasAttribute(a.name)) html_->setAttribute(a.name, a.value);
            return;
        }
        if (tag == "head") {
            if (!head_ && !body_) { ensureHtml(); head_ = insertElement(tok); inHead_ = true; }
            return;
        }
        if (tag == "body") {
            if (!body_) {
                ensureHead(); closeHead();
                while (!stack_.empty() && stack_.back() != html_) stack_.pop_back();
                body_ = insertElement(tok);
            } else {
                for (const auto& a : tok.attrs) if (!body_->hasAttribute(a.name)) body_->setAttribute(a.name, a.value);
            }
            return;
        }
        if (!body_ && inList(kHeadTags, tag)) {
            ensureHead();
            insertElement(tok);
            if (isVoidElement(tag) || tok.selfClosing) stack_.pop_back();
            return;
        }
        ensureBody();
        closeImpliedForStart(tag);
        insertElement(tok);
        if (isVoidElement(tag) || tok.selfClosing) stack_.pop_back();
    }

    void onEnd(const HtmlToken& tok) {
        const std::string& tag = tok.data;
        if (tag == "html" || tag == "body" || tag == "head") {
            if (tag == "head") closeHead();
            return;  // closing is deferred to end of input, as in the specification
        }
        if (tag == "br") { HtmlToken t; t.type = HtmlTokenType::StartTag; t.data = "br"; onStart(t); return; }
        if (tag == "p" && !inScope("p")) {
            ensureBody();
            HtmlToken t; t.type = HtmlTokenType::StartTag; t.data = "p";
            insertElement(t);
            stack_.pop_back();
            return;
        }
        if (inScope(tag)) popUntil(tag);
        // otherwise: stray end tag, ignored
    }

    std::unique_ptr<Document> doc_;
    std::vector<Element*> stack_;
    Element* html_ = nullptr;
    Element* head_ = nullptr;
    Element* body_ = nullptr;
    bool inHead_ = false;
};

} // namespace

bool isVoidElement(std::string_view tag) { return inList(kVoid, tag); }

std::unique_ptr<Document> parseHtml(std::string_view html) {
    TreeBuilder b;
    return b.build(html);
}

} // namespace HB
