#include "test_support.h"
#include "engine/url.hpp"
using namespace HB;

TEST(Url, ParseAndSerializeRoundTrip) {
    auto u = parseUrl("HTTP://example.com:8080/a/b?x=1#frag");
    EXPECT_EQ(u.scheme, std::string("http"));
    EXPECT_EQ(u.authority, std::string("example.com:8080"));
    EXPECT_EQ(u.path, std::string("/a/b"));
    EXPECT_EQ(u.query, std::string("x=1"));
    EXPECT_EQ(u.fragment, std::string("frag"));
    EXPECT_EQ(serializeUrl(parseUrl("file:///home/u/a.html#x")), std::string("file:///home/u/a.html#x"));
}

TEST(Url, ResolveRelativeReferences) {
    const std::string base = "file:///home/u/site/index.html";
    EXPECT_EQ(resolveUrl(base, "page.html"), std::string("file:///home/u/site/page.html"));
    EXPECT_EQ(resolveUrl(base, "../img/a.png"), std::string("file:///home/u/img/a.png"));
    EXPECT_EQ(resolveUrl(base, "./x/../y.css"), std::string("file:///home/u/site/y.css"));
    EXPECT_EQ(resolveUrl(base, "/etc/x.html"), std::string("file:///etc/x.html"));
    EXPECT_EQ(resolveUrl(base, "#top"), std::string("file:///home/u/site/index.html#top"));
    EXPECT_EQ(resolveUrl("file:///a/b.html#old", "#new"), std::string("file:///a/b.html#new"));
    EXPECT_EQ(resolveUrl(base, ""), base);
    EXPECT_EQ(resolveUrl(base, "https://other.org/x"), std::string("https://other.org/x"));
    EXPECT_EQ(resolveUrl(base, "mailto:a@b.c"), std::string("mailto:a@b.c"));
    EXPECT_EQ(resolveUrl(base, "../../../../../x.html"), std::string("file:///x.html"));
}

TEST(Url, HttpReferences) {
    const std::string base = "https://ex.com/a/b/c.html?q=1";
    EXPECT_EQ(resolveUrl(base, "d.html"), std::string("https://ex.com/a/b/d.html"));
    EXPECT_EQ(resolveUrl(base, "?z=2"), std::string("https://ex.com/a/b/c.html?z=2"));
    EXPECT_EQ(resolveUrl(base, "//cdn.net/x.js"), std::string("https://cdn.net/x.js"));
    EXPECT_EQ(resolveUrl(base, "../"), std::string("https://ex.com/a/"));
}

TEST(Url, NonHierarchicalBasesCannotResolveRelativeLinks) {
    EXPECT_EQ(resolveUrl("about:blank", "x.html"), std::string());
    EXPECT_EQ(resolveUrl("data:text/html,hi", "x.html"), std::string());
    EXPECT_EQ(resolveUrl("about:blank", "#a"), std::string("about:blank#a"));
}

TEST(Url, FragmentHelpers) {
    EXPECT_EQ(stripFragment("a#b"), std::string("a"));
    EXPECT_EQ(urlFragment("a#b%20c"), std::string("b%20c"));
    EXPECT_EQ(percentDecodeString("a%20b%zz"), std::string("a b%zz"));
    EXPECT_EQ(removeDotSegments("/a/./b/../c/"), std::string("/a/c/"));
}
