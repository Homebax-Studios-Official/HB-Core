#include "test_support.h"
#include "dom/node.hpp"
using namespace HB;

TEST(Dom, TreeOperations) {
    Document doc;
    auto* html = static_cast<Element*>(doc.appendChild(std::make_unique<Element>("html")));
    auto* body = static_cast<Element*>(html->appendChild(std::make_unique<Element>("body")));
    auto* a = static_cast<Element*>(body->appendChild(std::make_unique<Element>("a")));
    auto* b = static_cast<Element*>(body->appendChild(std::make_unique<Element>("b")));
    a->setAttribute("id", "first");
    a->setAttribute("class", "x  y");
    EXPECT_EQ(doc.documentElement(), html);
    EXPECT_EQ(doc.body(), body);
    EXPECT_EQ(a->nextElementSibling(), b);
    EXPECT_EQ(b->previousElementSibling(), a);
    EXPECT_EQ(doc.getElementById("first"), a);
    EXPECT_TRUE(a->hasClass("y"));
    EXPECT_FALSE(a->hasClass("z"));
    EXPECT_EQ(doc.getElementsByTagName("*").size(), size_t(4));
    a->setAttribute("id", "second");
    EXPECT_EQ(a->attribute("id"), std::string("second"));
    auto removed = body->removeChild(a);
    ASSERT_TRUE(removed != nullptr);
    EXPECT_TRUE(removed->parent() == nullptr);
    EXPECT_EQ(body->children().size(), size_t(1));
}

TEST(Dom, TextContentAndTitle) {
    Document doc;
    auto* html = static_cast<Element*>(doc.appendChild(std::make_unique<Element>("html")));
    auto* head = static_cast<Element*>(html->appendChild(std::make_unique<Element>("head")));
    auto* title = static_cast<Element*>(head->appendChild(std::make_unique<Element>("title")));
    title->appendChild(std::make_unique<Text>("  Hello \n  World "));
    html->appendChild(std::make_unique<Comment>("ignored"));
    EXPECT_EQ(doc.title(), std::string("Hello World"));
    EXPECT_EQ(doc.textContent(), std::string("  Hello \n  World "));
}
