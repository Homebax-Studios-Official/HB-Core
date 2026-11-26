// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace HB {

enum class NodeType { Document, DocumentType, Element, Text, Comment };

class Element;
class Document;

class Node {
public:
    virtual ~Node() = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    NodeType nodeType() const { return type_; }
    Node* parent() const { return parent_; }
    const std::vector<std::unique_ptr<Node>>& children() const { return children_; }

    // Takes ownership. Returns the raw pointer of the appended child.
    Node* appendChild(std::unique_ptr<Node> child);
    // Removes and returns the child (ownership transferred to caller), or nullptr if not a child.
    std::unique_ptr<Node> removeChild(Node* child);

    Node* firstChild() const { return children_.empty() ? nullptr : children_.front().get(); }
    Node* lastChild() const { return children_.empty() ? nullptr : children_.back().get(); }
    Node* nextSibling() const;
    Node* previousSibling() const;

    bool isElement() const { return type_ == NodeType::Element; }
    bool isText() const { return type_ == NodeType::Text; }
    Element* asElement();
    const Element* asElement() const;

    // Concatenated text of all descendant text nodes.
    std::string textContent() const;

protected:
    explicit Node(NodeType t) : type_(t) {}

private:
    NodeType type_;
    Node* parent_ = nullptr;
    std::vector<std::unique_ptr<Node>> children_;
};

struct Attribute {
    std::string name;  // lower-case
    std::string value;
};

class Element : public Node {
public:
    explicit Element(std::string tagName) : Node(NodeType::Element), tag_(std::move(tagName)) {}

    const std::string& tagName() const { return tag_; }  // lower-case
    const std::vector<Attribute>& attributes() const { return attrs_; }

    bool hasAttribute(std::string_view name) const;
    const std::string* getAttribute(std::string_view name) const;
    std::string attribute(std::string_view name) const;  // empty string if absent
    void setAttribute(std::string name, std::string value);

    std::string id() const { return attribute("id"); }
    std::vector<std::string> classList() const;
    bool hasClass(std::string_view cls) const;

    Element* firstElementChild() const;
    Element* nextElementSibling() const;
    Element* previousElementSibling() const;

private:
    std::string tag_;
    std::vector<Attribute> attrs_;
};

class Text : public Node {
public:
    explicit Text(std::string data) : Node(NodeType::Text), data_(std::move(data)) {}
    const std::string& data() const { return data_; }
    void append(std::string_view more) { data_.append(more); }
private:
    std::string data_;
};

class Comment : public Node {
public:
    explicit Comment(std::string data) : Node(NodeType::Comment), data_(std::move(data)) {}
    const std::string& data() const { return data_; }
private:
    std::string data_;
};

class DocumentType : public Node {
public:
    explicit DocumentType(std::string name) : Node(NodeType::DocumentType), name_(std::move(name)) {}
    const std::string& name() const { return name_; }
private:
    std::string name_;
};

class Document : public Node {
public:
    Document() : Node(NodeType::Document) {}

    Element* documentElement() const;
    Element* head() const;
    Element* body() const;
    std::string title() const;  // text of first <title>, whitespace-collapsed and trimmed

    Element* getElementById(std::string_view id) const;
    std::vector<Element*> getElementsByTagName(std::string_view tag) const;

    bool hasDoctype = false;
    bool quirksMode = true;
};

// Depth-first pre-order traversal over elements below (and excluding) root.
template <class F>
void forEachElement(const Node* root, F&& fn) {
    for (const auto& c : root->children()) {
        if (c->isElement()) fn(c->asElement());
        forEachElement(c.get(), fn);
    }
}

} // namespace HB
