// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "dom/node.hpp"

namespace HB {

enum class HtmlTokenType { Doctype, StartTag, EndTag, Character, Comment, EndOfFile };

struct HtmlToken {
    HtmlTokenType type = HtmlTokenType::EndOfFile;
    std::string data;                 // tag name (lower-case), doctype name, text or comment body
    std::vector<Attribute> attrs;     // start tags only; duplicate names dropped (first wins)
    bool selfClosing = false;
};

// Hand-written HTML tokenizer. It follows the WHATWG tokenization behaviour for the common
// constructs (tags, attributes, comments, doctype, RCDATA/RAWTEXT elements, character
// references) but is NOT a line-by-line implementation of the specification state machine.
// See SUPPORT_MATRIX.md for known deviations.
class HtmlTokenizer {
public:
    explicit HtmlTokenizer(std::string_view input) : in_(input) {}

    // Returns all tokens, ending with a single EndOfFile token. Adjacent text is coalesced.
    std::vector<HtmlToken> tokenize();

private:
    void emitText(std::string text);
    void readTag(std::vector<HtmlToken>& out);
    void readComment(std::vector<HtmlToken>& out);
    void readDoctype(std::vector<HtmlToken>& out);
    void readRawText(const std::string& tag, bool decodeEntities, std::vector<HtmlToken>& out);

    std::string_view in_;
    size_t pos_ = 0;
    std::string text_;
};

// Decodes character references (&amp; &#65; &#x41; and a table of common named references).
std::string decodeHtmlEntities(std::string_view s);

} // namespace HB
