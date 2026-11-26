#include "test_support.h"
#include "html/parser.hpp"
#include "html/tokenizer.hpp"
using namespace HB;

TEST(Tokenizer, TagsAttributesAndText) {
    auto t = HtmlTokenizer("<DIV Class=\"a b\" id=x data-k='v' hidden>Hi &amp; bye</div>").tokenize();
    ASSERT_EQ(t.size(), size_t(4));
    EXPECT_TRUE(t[0].type == HtmlTokenType::StartTag);
    EXPECT_EQ(t[0].data, std::string("div"));
    ASSERT_EQ(t[0].attrs.size(), size_t(4));
    EXPECT_EQ(t[0].attrs[0].value, std::string("a b"));
    EXPECT_EQ(t[0].attrs[1].value, std::string("x"));
    EXPECT_EQ(t[0].attrs[2].value, std::string("v"));
    EXPECT_EQ(t[0].attrs[3].name, std::string("hidden"));
    EXPECT_EQ(t[1].data, std::string("Hi & bye"));
    EXPECT_TRUE(t[2].type == HtmlTokenType::EndTag);
}

TEST(Tokenizer, DuplicateAttributeFirstWins) {
    auto t = HtmlTokenizer("<p a=1 a=2>").tokenize();
    ASSERT_EQ(t[0].attrs.size(), size_t(1));
    EXPECT_EQ(t[0].attrs[0].value, std::string("1"));
}

TEST(Tokenizer, CommentDoctypeAndSelfClosing) {
    auto t = HtmlTokenizer("<!DOCTYPE html><!-- c --><br/>").tokenize();
    EXPECT_TRUE(t[0].type == HtmlTokenType::Doctype);
    EXPECT_EQ(t[0].data, std::string("html"));
    EXPECT_TRUE(t[1].type == HtmlTokenType::Comment);
    EXPECT_EQ(t[1].data, std::string(" c "));
    EXPECT_TRUE(t[2].selfClosing);
}

TEST(Tokenizer, RawTextAndRcdata) {
    auto t = HtmlTokenizer("<style>a<b{}</style><title>x &lt; y</title>").tokenize();
    EXPECT_EQ(t[1].data, std::string("a<b{}"));
    EXPECT_EQ(t[4].data, std::string("x < y"));
}

TEST(Tokenizer, StrayLessThanIsText) {
    auto t = HtmlTokenizer("a < b").tokenize();
    ASSERT_EQ(t.size(), size_t(2));
    EXPECT_EQ(t[0].data, std::string("a < b"));
}

TEST(Entities, NumericAndNamed) {
    EXPECT_EQ(decodeHtmlEntities("&#65;&#x42;&copy;&unknown;&amp"), std::string("AB\xC2\xA9&unknown;&amp"));
    EXPECT_EQ(decodeHtmlEntities("&#0;"), std::string("\xEF\xBF\xBD"));
}

TEST(Parser, MinimalPageBuildsExpectedTree) {
    auto doc = parseHtml("<!DOCTYPE html><html><head><title>HB Core Test</title></head><body><h1>Hello HB Core</h1><p>Independent browser engine.</p></body></html>");
    ASSERT_TRUE(doc->body() != nullptr);
    EXPECT_TRUE(doc->hasDoctype);
    EXPECT_FALSE(doc->quirksMode);
    EXPECT_EQ(doc->title(), std::string("HB Core Test"));
    auto h1 = doc->getElementsByTagName("h1");
    ASSERT_EQ(h1.size(), size_t(1));
    EXPECT_EQ(h1[0]->textContent(), std::string("Hello HB Core"));
}

TEST(Parser, ImpliedStructureFromFragment) {
    auto doc = parseHtml("<p>x");
    ASSERT_TRUE(doc->documentElement() != nullptr);
    ASSERT_TRUE(doc->head() != nullptr);
    ASSERT_TRUE(doc->body() != nullptr);
    EXPECT_EQ(doc->body()->textContent(), std::string("x"));
    EXPECT_EQ(parseHtml("")->body() != nullptr, true);
}

TEST(Parser, ImpliedEndTags) {
    auto doc = parseHtml("<body><p>a<p>b<ul><li>1<li>2</ul><div>c</div>");
    EXPECT_EQ(doc->getElementsByTagName("p").size(), size_t(2));
    auto li = doc->getElementsByTagName("li");
    ASSERT_EQ(li.size(), size_t(2));
    EXPECT_TRUE(li[0]->parent() == li[1]->parent());
    auto divs = doc->getElementsByTagName("div");
    EXPECT_EQ(divs[0]->parent()->asElement()->tagName(), std::string("body"));
}

TEST(Parser, VoidElementsHaveNoChildren) {
    auto doc = parseHtml("<body>a<br>b<img src=x>c<hr>d");
    EXPECT_EQ(doc->getElementsByTagName("br")[0]->children().size(), size_t(0));
    EXPECT_EQ(doc->body()->children().size(), size_t(7));
}

TEST(Parser, StrayEndTagsIgnoredAndStrayPCreated) {
    auto doc = parseHtml("<body></span>text</p>");
    EXPECT_EQ(doc->getElementsByTagName("span").size(), size_t(0));
    EXPECT_EQ(doc->getElementsByTagName("p").size(), size_t(1));
}

TEST(Parser, ScriptContentIsNotParsedAsMarkup) {
    auto doc = parseHtml("<body><script>if (a<b) { x = '<p>'; }</script><p>ok</p></body>");
    EXPECT_EQ(doc->getElementsByTagName("p").size(), size_t(1));
    EXPECT_EQ(doc->getElementsByTagName("script")[0]->textContent(), std::string("if (a<b) { x = '<p>'; }"));
}

TEST(Parser, DeepNestingDoesNotCrash) {
    std::string s = "<body>";
    for (int i = 0; i < 3000; ++i) s += "<div>";
    s += "x";
    auto doc = parseHtml(s);
    EXPECT_EQ(doc->getElementsByTagName("div").size(), size_t(3000));
}

TEST(Parser, GarbageInputSurvives) {
    std::string junk;
    for (int i = 0; i < 4000; ++i) junk.push_back(static_cast<char>((i * 131 + 7) % 256));
    junk += "<<<<</><!--<!DOCTYPE<a b=\"<c";
    auto doc = parseHtml(junk);
    EXPECT_TRUE(doc->body() != nullptr);
}
