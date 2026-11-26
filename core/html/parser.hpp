// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <memory>
#include <string_view>
#include "dom/node.hpp"

namespace HB {

// Builds a DOM tree from HTML text. Always returns a document with <html>, <head> and <body>.
//
// Implemented: implied html/head/body, void elements, implied end tags for p/li/dt/dd/option/
// tr/td/th/head, end-tag matching with scope barriers, comments, doctype, text merging.
// NOT implemented: the adoption agency algorithm / active formatting elements, table foster
// parenting, foreign content (SVG/MathML), templates, scripting-driven parsing, encodings other
// than UTF-8. See SUPPORT_MATRIX.md.
std::unique_ptr<Document> parseHtml(std::string_view html);

bool isVoidElement(std::string_view tag);

} // namespace HB
