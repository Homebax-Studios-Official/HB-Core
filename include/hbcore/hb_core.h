// HB Core - public C++ API. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
// Licensed under the Homebax Studios Non-Commercial License (see LICENSE).
//
// The C++ API uses std::string/std::function and is therefore only ABI-compatible between
// binaries built with the same compiler and standard library. For a stable ABI use hb_core_c.h.
#ifndef HBCORE_HB_CORE_H
#define HBCORE_HB_CORE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "hb_error.h"
#include "hb_export.h"
#include "hb_version.h"

namespace HB {

struct CoreConfig {
    bool enableJavaScript = true;   // accepted; no JavaScript engine exists in 0.1.0
    bool enableGPU = false;         // accepted; only the CPU renderer exists in 0.1.0
    uint32_t memoryLimitMB = 512;   // upper bound for documents; a document may use at most 1/8 of it as source text
    int logLevel = 2;               // 0 trace, 1 debug, 2 info, 3 warning, 4 error, 5 off
};

struct ViewConfig {
    int width = 800;                // CSS pixels
    int height = 600;
    float devicePixelRatio = 1.0f;
};

enum class ViewEventType {
    NavigationStarted,
    NavigationCompleted,
    NavigationFailed,
    TitleChanged,
    UrlChanged,
    Error
};

struct ViewEvent {
    ViewEventType type = ViewEventType::Error;
    std::string text;   // url, title or message
    ErrorCode code = ErrorCode::Ok;
};

// Read-only view of the rendered RGBA8 pixels. Valid until the next render()/resize on the view.
struct Framebuffer {
    int width = 0;
    int height = 0;
    int strideBytes = 0;
    const uint8_t* pixels = nullptr;
};

class View;

class HB_API Core {
public:
    Core();
    ~Core();
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

    Error initialize(const CoreConfig& config);
    bool isInitialized() const;
    const CoreConfig& config() const;
    void shutdown();

    static const char* version();

private:
    friend class View;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class HB_API View {
public:
    explicit View(Core& core, const ViewConfig& config = ViewConfig());
    ~View();
    View(const View&) = delete;
    View& operator=(const View&) = delete;

    // Navigation. Supported URL schemes in 0.1.0: file://, data:text/html, about:blank.
    Error loadURL(const std::string& url);
    Error loadHTML(const std::string& html);
    Error goBack();
    Error goForward();
    Error reload();
    bool canGoBack() const;
    bool canGoForward() const;

    // Returns ErrorCode::Unsupported: HB.JavaScript does not exist yet.
    Error executeJavaScript(const std::string& script, std::string* result = nullptr);

    void resize(int width, int height);
    void setDevicePixelRatio(float dpr);
    void scrollBy(float dyCssPixels);
    void scrollTo(float yCssPixels);
    float scrollY() const;
    float documentHeight();

    std::string title() const;
    std::string url() const;

    // Renders into the view's internal framebuffer (CPU renderer).
    Framebuffer render();
    Error savePNG(const std::string& path);

    void setEventHandler(std::function<void(const ViewEvent&)> handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace HB

#endif
