// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "engine/page.hpp"
#include "base/strings.hpp"
#include "engine/url.hpp"
#include "html/parser.hpp"
#include <algorithm>
#include <cmath>

namespace HB {

Page::Page() { setSource("", {}); }

void Page::setSource(const std::string& html, ResourceLoader loader) {
    doc_ = parseHtml(html);
    sheets_.clear();
    imageCache_.clear();
    loader_ = std::move(loader);
    forEachElement(doc_.get(), [&](Element* el) {
        if (el->tagName() == "style") {
            std::string type = toLowerAscii(el->attribute("type"));
            if (type.empty() || type == "text/css")
                sheets_.push_back(std::make_unique<Stylesheet>(parseStylesheet(el->textContent())));
        } else if (el->tagName() == "link" && equalsIgnoreCase(el->attribute("rel"), "stylesheet") && loader_) {
            if (auto css = loader_(el->attribute("href")))
                sheets_.push_back(std::make_unique<Stylesheet>(parseStylesheet(*css)));
        }
    });
    layoutDirty_ = true;
    layout_.reset();
    list_.reset();
    scrollY_ = 0;
}

std::shared_ptr<const Image> Page::loadImage(const std::string& src) {
    if (src.empty()) return nullptr;
    auto it = imageCache_.find(src);
    if (it != imageCache_.end()) return it->second;
    std::shared_ptr<const Image> result;
    std::string bytes;
    bool have = false;
    if (startsWith(toLowerAscii(src), "data:")) {
        size_t comma = src.find(',');
        if (comma != std::string::npos) {
            std::string head = toLowerAscii(src.substr(0, comma));
            if (head.find(";base64") != std::string::npos) { bool ok = false; bytes = base64Decode(std::string_view(src).substr(comma + 1), &ok); have = ok; }
            else { bytes = percentDecodeString(src.substr(comma + 1)); have = true; }
        }
    } else if (loader_) {
        if (auto data = loader_(src)) { bytes = std::move(*data); have = true; }
    }
    if (have) {
        if (auto img = decodeImage(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()))
            result = std::make_shared<const Image>(std::move(*img));
    }
    imageCache_[src] = result;
    return result;
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
    layout_ = buildLayout(*doc_, resolver, vp, [this](const std::string& src) { return loadImage(src); });
    list_ = std::make_unique<DisplayList>(buildDisplayList(*layout_));
    layoutDirty_ = false;
}

const LayoutTree* Page::layout() { ensureLayout(); return layout_.get(); }
const DisplayList* Page::displayList() { ensureLayout(); return list_.get(); }

float Page::documentHeight() { ensureLayout(); return layout_->documentHeight; }

void Page::setScrollY(float y) {
    float maxScroll = layout_ ? std::max(0.0f, layout_->documentHeight - viewH_) : 1e9f;
    scrollY_ = std::clamp(y, 0.0f, maxScroll);
}

HitTestResult Page::hitTest(float viewX, float viewY) {
    ensureLayout();
    return HB::hitTest(*layout_, viewX, viewY + scrollY_);
}

bool Page::scrollToFragment(const std::string& fragment) {
    if (fragment.empty()) { setScrollY(0); return true; }
    ensureLayout();
    std::string name = percentDecodeString(fragment);
    const Element* target = doc_->getElementById(name);
    if (!target) {
        for (Element* a : doc_->getElementsByTagName("a"))
            if (a->attribute("name") == name) { target = a; break; }
    }
    float x, y;
    if (!target || !elementPosition(*layout_, target, x, y)) return false;
    setScrollY(y);
    return true;
}

const Surface& Page::render() {
    ensureLayout();
    scrollY_ = std::clamp(scrollY_, 0.0f, std::max(0.0f, layout_->documentHeight - viewH_));
    int pw = std::max(1, static_cast<int>(std::lround(viewW_ * dpr_)));
    int ph = std::max(1, static_cast<int>(std::lround(viewH_ * dpr_)));
    if (surface_.width() != pw || surface_.height() != ph) surface_.resize(pw, ph);
    rasterize(*list_, surface_, scrollY_, dpr_);
    return surface_;
}

} // namespace HB
