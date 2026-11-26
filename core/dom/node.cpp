// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "dom/node.hpp"
#include "base/strings.hpp"
#include <algorithm>

namespace HB {

Node* Node::appendChild(std::unique_ptr<Node> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
    return children_.back().get();
}

std::unique_ptr<Node> Node::removeChild(Node* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            std::unique_ptr<Node> out = std::move(*it);
            children_.erase(it);
            out->parent_ = nullptr;
            return out;
        }
    }
    return nullptr;
}

Node* Node::nextSibling() const {
    if (!parent_) return nullptr;
    const auto& sib = parent_->children_;
    for (size_t i = 0; i < sib.size(); ++i)
        if (sib[i].get() == this) return i + 1 < sib.size() ? sib[i + 1].get() : nullptr;
    return nullptr;
}

Node* Node::previousSibling() const {
    if (!parent_) return nullptr;
    const auto& sib = parent_->children_;
    for (size_t i = 0; i < sib.size(); ++i)
        if (sib[i].get() == this) return i > 0 ? sib[i - 1].get() : nullptr;
    return nullptr;
}

Element* Node::asElement() { return isElement() ? static_cast<Element*>(this) : nullptr; }
const Element* Node::asElement() const { return isElement() ? static_cast<const Element*>(this) : nullptr; }

std::string Node::textContent() const {
    std::string out;
    if (type_ == NodeType::Text) return static_cast<const Text*>(this)->data();
    for (const auto& c : children_) {
        if (c->nodeType() == NodeType::Comment) continue;
        out += c->textContent();
    }
    return out;
}

bool Element::hasAttribute(std::string_view name) const { return getAttribute(name) != nullptr; }

const std::string* Element::getAttribute(std::string_view name) const {
    for (const auto& a : attrs_) if (a.name == name) return &a.value;
    return nullptr;
}

std::string Element::attribute(std::string_view name) const {
    const std::string* v = getAttribute(name);
    return v ? *v : std::string();
}

void Element::setAttribute(std::string name, std::string value) {
    for (auto& a : attrs_) {
        if (a.name == name) { a.value = std::move(value); return; }
    }
    attrs_.push_back({std::move(name), std::move(value)});
}

std::vector<std::string> Element::classList() const { return splitWhitespace(attribute("class")); }

bool Element::hasClass(std::string_view cls) const {
    for (const auto& c : classList()) if (c == cls) return true;
    return false;
}

Element* Element::firstElementChild() const {
    for (const auto& c : children()) if (c->isElement()) return c->asElement();
    return nullptr;
}

Element* Element::nextElementSibling() const {
    for (Node* n = nextSibling(); n; n = n->nextSibling()) if (n->isElement()) return n->asElement();
    return nullptr;
}

Element* Element::previousElementSibling() const {
    for (Node* n = previousSibling(); n; n = n->previousSibling()) if (n->isElement()) return n->asElement();
    return nullptr;
}

Element* Document::documentElement() const {
    for (const auto& c : children()) if (c->isElement()) return c->asElement();
    return nullptr;
}

Element* Document::head() const {
    Element* html = documentElement();
    if (!html) return nullptr;
    for (const auto& c : html->children()) if (c->isElement() && c->asElement()->tagName() == "head") return c->asElement();
    return nullptr;
}

Element* Document::body() const {
    Element* html = documentElement();
    if (!html) return nullptr;
    for (const auto& c : html->children()) if (c->isElement() && c->asElement()->tagName() == "body") return c->asElement();
    return nullptr;
}

std::string Document::title() const {
    std::string raw;
    bool found = false;
    forEachElement(this, [&](Element* e) {
        if (!found && e->tagName() == "title") { raw = e->textContent(); found = true; }
    });
    std::string out;
    bool pendingSpace = false;
    for (char c : raw) {
        if (isAsciiSpace(c)) { pendingSpace = !out.empty(); continue; }
        if (pendingSpace) { out.push_back(' '); pendingSpace = false; }
        out.push_back(c);
    }
    return out;
}

Element* Document::getElementById(std::string_view id) const {
    Element* result = nullptr;
    forEachElement(this, [&](Element* e) {
        if (!result) { const std::string* v = e->getAttribute("id"); if (v && *v == id) result = e; }
    });
    return result;
}

std::vector<Element*> Document::getElementsByTagName(std::string_view tag) const {
    std::vector<Element*> out;
    std::string t = toLowerAscii(tag);
    forEachElement(this, [&](Element* e) { if (t == "*" || e->tagName() == t) out.push_back(e); });
    return out;
}

} // namespace HB
