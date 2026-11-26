// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "dom/node.hpp"

namespace HB {

struct Declaration {
    std::string property;  // lower-case, except custom properties (--x) which keep case
    std::string value;
    bool important = false;
};

struct AttributeSelector {
    std::string name;
    char op = 0;  // 0 = exists, '=', '~', '|', '^', '$', '*'
    std::string value;
};

struct CompoundSelector {
    std::string tag;  // empty or "*" = any
    std::string id;
    std::vector<std::string> classes;
    std::vector<AttributeSelector> attrs;
    std::vector<std::string> pseudoClasses;  // supported: first-child, last-child, only-child, root, empty
    bool unsupported = false;                // selector uses something we cannot evaluate -> never matches
};

enum class Combinator { Descendant, Child, NextSibling, SubsequentSibling };

struct ComplexSelector {
    std::vector<CompoundSelector> parts;      // left to right
    std::vector<Combinator> combinators;      // combinators[i] joins parts[i] and parts[i+1]
    uint32_t specificity = 0;                 // a<<16 | b<<8 | c
};

struct MediaCondition {
    bool never = false;  // unsupported media query -> rule never applies
    float minWidth = -1, maxWidth = -1;
    bool matches(float viewportWidth) const;
};

struct StyleRule {
    std::vector<ComplexSelector> selectors;
    std::vector<Declaration> declarations;
    MediaCondition media;
    uint32_t order = 0;
};

struct Stylesheet {
    std::vector<StyleRule> rules;
};

// Parses a stylesheet. Never fails: invalid constructs are skipped following CSS error recovery.
// Supported at-rules: @media (min-width/max-width, "screen"/"all"). Other at-rules are skipped.
Stylesheet parseStylesheet(std::string_view css);
std::vector<Declaration> parseDeclarations(std::string_view block);
std::vector<ComplexSelector> parseSelectorList(std::string_view text);
bool selectorMatches(const ComplexSelector& sel, const Element& el);

} // namespace HB
