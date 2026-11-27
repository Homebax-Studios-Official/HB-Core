// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <string>

namespace HB {

struct UrlParts {
    std::string scheme;     // lower-case, without ':'
    std::string authority;  // text after "//" up to the path, may be empty
    std::string path;       // starts with '/' for hierarchical URLs
    std::string query;      // without '?'
    std::string fragment;   // without '#'
    bool hierarchical = false;  // has "//" authority section or an absolute path
    bool hasQuery = false;
    bool hasFragment = false;
};

UrlParts parseUrl(const std::string& url);
std::string serializeUrl(const UrlParts& u);
// RFC 3986-style reference resolution (subset). Returns "" if `base` is not hierarchical (about:, data:)
// and `ref` is relative.
std::string resolveUrl(const std::string& base, const std::string& ref);
std::string removeDotSegments(const std::string& path);
std::string stripFragment(const std::string& url);
std::string urlFragment(const std::string& url);  // decoded-free fragment text (without '#')
std::string percentDecodeString(const std::string& s);

} // namespace HB
