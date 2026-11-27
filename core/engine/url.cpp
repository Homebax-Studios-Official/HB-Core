// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "engine/url.hpp"
#include "base/strings.hpp"
#include <cctype>
#include <vector>

namespace HB {

namespace {

bool schemeEnd(const std::string& s, size_t& colon) {
    if (s.empty() || !isAsciiAlpha(s[0])) return false;
    for (size_t i = 1; i < s.size(); ++i) {
        char c = s[i];
        if (c == ':') { colon = i; return true; }
        if (!(isAsciiAlnum(c) || c == '+' || c == '-' || c == '.')) return false;
    }
    return false;
}

}  // namespace

std::string percentDecodeString(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() + 0 && std::isxdigit(static_cast<unsigned char>(s[i + 1])) && std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
            out.push_back(static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16)));
            i += 2;
        } else out.push_back(s[i]);
    }
    return out;
}

UrlParts parseUrl(const std::string& url) {
    UrlParts u;
    std::string rest = url;
    size_t hash = rest.find('#');
    if (hash != std::string::npos) { u.fragment = rest.substr(hash + 1); u.hasFragment = true; rest.erase(hash); }
    size_t colon;
    if (schemeEnd(rest, colon)) { u.scheme = toLowerAscii(rest.substr(0, colon)); rest.erase(0, colon + 1); }
    size_t q = rest.find('?');
    if (q != std::string::npos) { u.query = rest.substr(q + 1); u.hasQuery = true; rest.erase(q); }
    if (startsWith(rest, "//")) {
        u.hierarchical = true;
        size_t slash = rest.find('/', 2);
        u.authority = rest.substr(2, slash == std::string::npos ? std::string::npos : slash - 2);
        u.path = slash == std::string::npos ? "" : rest.substr(slash);
    } else {
        u.path = rest;
        u.hierarchical = !rest.empty() && rest[0] == '/';
    }
    return u;
}

std::string serializeUrl(const UrlParts& u) {
    std::string out;
    if (!u.scheme.empty()) out += u.scheme + ":";
    if (u.hierarchical && (!u.authority.empty() || u.scheme == "file" || startsWith(u.path, "//") == false)) {
        if (!u.authority.empty() || u.scheme == "file") out += "//" + u.authority;
    }
    out += u.path;
    if (u.hasQuery) out += "?" + u.query;
    if (u.hasFragment) out += "#" + u.fragment;
    return out;
}

std::string removeDotSegments(const std::string& path) {
    std::vector<std::string> out;
    size_t i = 0;
    bool trailingSlash = false;
    while (i <= path.size()) {
        size_t j = path.find('/', i);
        std::string seg = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
        bool last = j == std::string::npos;
        if (seg == "..") { if (out.size() > 1 || (out.size() == 1 && !out[0].empty())) out.pop_back(); trailingSlash = last; }
        else if (seg == ".") { trailingSlash = last; }
        else { out.push_back(seg); trailingSlash = false; }
        if (last) break;
        i = j + 1;
    }
    std::string r;
    for (size_t k = 0; k < out.size(); ++k) { if (k) r += "/"; r += out[k]; }
    if (trailingSlash && (r.empty() || r.back() != '/')) r += "/";
    return r;
}

std::string stripFragment(const std::string& url) {
    size_t h = url.find('#');
    return h == std::string::npos ? url : url.substr(0, h);
}

std::string urlFragment(const std::string& url) {
    size_t h = url.find('#');
    return h == std::string::npos ? std::string() : url.substr(h + 1);
}

std::string resolveUrl(const std::string& base, const std::string& rawRef) {
    std::string ref(trim(rawRef));
    UrlParts b = parseUrl(base);
    size_t colon;
    if (!ref.empty() && schemeEnd(ref, colon)) return ref;  // absolute
    if (ref.empty()) return stripFragment(base);
    if (ref[0] == '#') return stripFragment(base) + ref;
    if (b.scheme.empty() || (!b.hierarchical && b.scheme != "file")) return std::string();
    UrlParts r = parseUrl(ref);
    UrlParts t;
    t.scheme = b.scheme;
    t.hierarchical = true;
    if (startsWith(ref, "//")) {
        t.authority = r.authority; t.path = removeDotSegments(r.path);
        t.query = r.query; t.hasQuery = r.hasQuery;
    } else {
        t.authority = b.authority;
        if (!r.path.empty() && r.path[0] == '/') {
            t.path = removeDotSegments(r.path);
            t.query = r.query; t.hasQuery = r.hasQuery;
        } else if (r.path.empty()) {
            t.path = b.path;
            t.query = r.hasQuery ? r.query : b.query; t.hasQuery = r.hasQuery || b.hasQuery;
        } else {
            size_t slash = b.path.rfind('/');
            std::string dir = slash == std::string::npos ? "/" : b.path.substr(0, slash + 1);
            t.path = removeDotSegments(dir + r.path);
            t.query = r.query; t.hasQuery = r.hasQuery;
        }
    }
    t.fragment = r.fragment; t.hasFragment = r.hasFragment;
    return serializeUrl(t);
}

} // namespace HB
