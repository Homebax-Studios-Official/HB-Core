// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "css/style.hpp"
#include "dom/node.hpp"
#include "graphics/surface.hpp"
#include "layout/layout.hpp"
#include "paint/display_list.hpp"

namespace HB {

// Orchestrates the pipeline: HTML -> DOM -> stylesheets -> style -> layout -> display list -> pixels.
class Page {
public:
    // Loads a sub-resource (stylesheet text or image bytes) by href, relative to the page. May be empty.
    using ResourceLoader = std::function<std::optional<std::string>(const std::string& href)>;

    Page();

    // Parses the document and collects <style> / <link rel=stylesheet> sheets.
    void setSource(const std::string& html, ResourceLoader loader);
    void setViewport(float width, float height, float devicePixelRatio);
    void setScrollY(float y);

    const Document* document() const { return doc_.get(); }
    std::string title() const { return doc_ ? doc_->title() : std::string(); }
    float scrollY() const { return scrollY_; }
    float viewportHeight() const { return viewH_; }
    float documentHeight();
    const LayoutTree* layout();           // lays out if needed
    const DisplayList* displayList();     // paints if needed
    const Surface& render();              // rasterizes the current viewport

    // Viewport coordinates in CSS px (scroll offset is applied internally).
    HitTestResult hitTest(float viewX, float viewY);
    // Scrolls so that the element with this id (or <a name>) is at the top. Returns false if not found.
    bool scrollToFragment(const std::string& fragment);

private:
    void ensureLayout();
    std::shared_ptr<const Image> loadImage(const std::string& src);

    std::unique_ptr<Document> doc_;
    std::vector<std::unique_ptr<Stylesheet>> sheets_;
    std::unique_ptr<LayoutTree> layout_;
    std::unique_ptr<DisplayList> list_;
    ResourceLoader loader_;
    std::map<std::string, std::shared_ptr<const Image>> imageCache_;
    Surface surface_{1, 1};
    float viewW_ = 800, viewH_ = 600, dpr_ = 1.0f, scrollY_ = 0;
    bool layoutDirty_ = true;
};

} // namespace HB
