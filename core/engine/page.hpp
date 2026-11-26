// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <functional>
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
    using ResourceLoader = std::function<std::optional<std::string>(const std::string& href)>;

    Page();

    // Parses the document and collects <style> / <link rel=stylesheet> sheets.
    // `loader` resolves stylesheet hrefs (may be empty: external sheets are then skipped).
    void setSource(const std::string& html, ResourceLoader loader);
    void setViewport(float width, float height, float devicePixelRatio);
    void setScrollY(float y);

    const Document* document() const { return doc_.get(); }
    std::string title() const { return doc_ ? doc_->title() : std::string(); }
    float scrollY() const { return scrollY_; }
    float documentHeight();
    const LayoutTree* layout();           // lays out if needed
    const DisplayList* displayList();     // paints if needed
    const Surface& render();              // rasterizes the current viewport

private:
    void ensureLayout();

    std::unique_ptr<Document> doc_;
    std::vector<std::unique_ptr<Stylesheet>> sheets_;
    std::unique_ptr<LayoutTree> layout_;
    std::unique_ptr<DisplayList> list_;
    Surface surface_{1, 1};
    float viewW_ = 800, viewH_ = 600, dpr_ = 1.0f, scrollY_ = 0;
    bool layoutDirty_ = true;
};

} // namespace HB
