// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "engine/page.hpp"
#include "base/strings.hpp"
#include "html/parser.hpp"
#include <algorithm>
#include <cmath>

namespace HB {

Page::Page() { setSource("", {}); }

void Page::setSource(const std::string& html, ResourceLoader loader) {
    doc_ = parseHtml(html);
    sheets_.clear();
    forEachElement(doc_.get(), [&](Element* el) {
        if (el->tagName() == "style") {
            std::string type = toLowerAscii(el->attribute("type"));
            if (type.empty() || type == "text/css")
                sheets_.push_back(std::make_unique<Stylesheet>(parseStylesheet(el->textContent())));
        } else if (el->tagName() == "link" && equalsIgnoreCase(el->attribute("rel"), "stylesheet") && loader) {
            if (auto css = loader(el->attribute("href")))
                sheets_.push_back(std::make_unique<Stylesheet>(parseStylesheet(*css)));
        }
    });
    layoutDirty_ = true;
    scrollY_ = 0;
}

void Page::setViewport(float width, float height, float dpr) {
    width = std::max(1.0f, width);
    height = std::max(1.0f, height);
    dpr = std::clamp(dpr, 0.25f, 8.0f);
    if (width != viewW_) layoutDirty_ = true;
    viewW_ = width; viewH_ = height; dpr_ = dpr;
    setScrollY(scrollY_);
}

void Page::ensureLayout() {
    if (!layoutDirty_ && layout_) return;
    std::vector<const Stylesheet*> author;
    for (const auto& s : sheets_) author.push_back(s.get());
    ViewportInfo vp{viewW_, viewH_};
    StyleResolver resolver(&userAgentStylesheet(), author, vp);
    layout_ = buildLayout(*doc_, resolver, vp);
    list_ = std::make_unique<DisplayList>(buildDisplayList(*layout_));
    layoutDirty_ = false;
}

const LayoutTree* Page::layout() { ensureLayout(); return layout_.get(); }
const DisplayList* Page::displayList() { ensureLayout(); return list_.get(); }

float Page::documentHeight() { ensureLayout(); return layout_->documentHeight; }

void Page::setScrollY(float y) {
    float maxScroll = 0;
    if (!layoutDirty_ && layout_) maxScroll = std::max(0.0f, layout_->documentHeight - viewH_);
    else if (layout_) maxScroll = std::max(0.0f, layout_->documentHeight - viewH_);
    else maxScroll = 1e9f;
    scrollY_ = std::clamp(y, 0.0f, maxScroll);
}

const Surface& Page::render() {
    ensureLayout();
    // Re-clamp scroll against fresh layout.
    scrollY_ = std::clamp(scrollY_, 0.0f, std::max(0.0f, layout_->documentHeight - viewH_));
    int pw = std::max(1, static_cast<int>(std::lround(viewW_ * dpr_)));
    int ph = std::max(1, static_cast<int>(std::lround(viewH_ * dpr_)));
    if (surface_.width() != pw || surface_.height() != ph) surface_.resize(pw, ph);
    rasterize(*list_, surface_, scrollY_, dpr_);
    return surface_;
}

} // namespace HB
