// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "hbcore/hb_core.h"
#include "base/log.hpp"
#include "base/strings.hpp"
#include "engine/page.hpp"
#include "engine/url.hpp"
#include "graphics/fonts.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace HB {

namespace {

// Returns a filesystem path for file:// URLs (query and fragment removed).
std::string fileUrlToPath(const std::string& url) {
    UrlParts u = parseUrl(url);
    std::string p = percentDecodeString(u.path);
    if (p.size() >= 3 && p[0] == '/' && isAsciiAlpha(p[1]) && p[2] == ':') p.erase(0, 1);  // /C:/x -> C:/x
    return p;
}

bool readFile(const std::string& path, size_t maxBytes, std::string& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { err = "cannot open file: " + path; return false; }
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    if (out.size() > maxBytes) { err = "document exceeds the configured memory limit"; return false; }
    return true;
}

Error makeErr(ErrorCode c, std::string m) { return Error{c, std::move(m), }; }

} // namespace

// ---- Core ----

struct Core::Impl {
    CoreConfig config;
    bool initialized = false;
};

Core::Core() : impl_(std::make_unique<Impl>()) {}
Core::~Core() { shutdown(); }

Error Core::initialize(const CoreConfig& config) {
    if (impl_->initialized) return makeErr(ErrorCode::InvalidArgument, "Core is already initialized");
    if (config.memoryLimitMB == 0) return makeErr(ErrorCode::InvalidArgument, "memoryLimitMB must be > 0");
    impl_->config = config;
    int lvl = std::clamp(config.logLevel, 0, 5);
    Logger::instance().setLevel(static_cast<LogLevel>(lvl));
    FontProvider& fp = FontProvider::instance();
    if (!config.fontDirectory.empty()) fp.addSearchDirectory(config.fontDirectory);
    if (config.useSystemFonts && !fp.available()) fp.discoverSystemFonts();
    impl_->initialized = true;
    HB_LOG_INFO("HB Core " << HBCORE_VERSION_STRING << " initialized (CPU renderer); fonts: " << fp.describe());
    return Error{};
}

bool Core::isInitialized() const { return impl_->initialized; }
const CoreConfig& Core::config() const { return impl_->config; }
void Core::shutdown() { impl_->initialized = false; }
const char* Core::version() { return HBCORE_VERSION_STRING; }

bool Core::loadFont(const std::string& family, bool bold, bool italic, const std::string& path) {
    std::string f = toLowerAscii(family);
    FontFamily fam;
    if (f == "sans" || f == "sans-serif") fam = FontFamily::Sans;
    else if (f == "serif") fam = FontFamily::Serif;
    else if (f == "mono" || f == "monospace") fam = FontFamily::Mono;
    else return false;
    return FontProvider::instance().loadFile(fam, bold, italic, path);
}

std::string Core::fontInfo() const { return FontProvider::instance().describe(); }

// ---- View ----

struct HistoryEntry {
    std::string url;       // includes the #fragment
    std::string html;      // snapshot for non-file entries
    bool isFile = false;
    std::string path;
    float scrollY = 0;
};

struct View::Impl {
    Core* core;
    ViewConfig config;
    Page page;
    std::vector<HistoryEntry> history;
    int index = -1;
    std::string url = "about:blank";
    std::string title;
    std::function<void(const ViewEvent&)> handler;
    bool focused = false;
    CursorType cursor = CursorType::Arrow;
    std::string hoverLink;
    const Element* downLink = nullptr;
    MouseButton downButton = MouseButton::Left;

    void emit(ViewEventType t, const std::string& text, ErrorCode c = ErrorCode::Ok, float x = 0, float y = 0) {
        if (handler) handler(ViewEvent{t, text, c, x, y});
    }

    size_t maxSourceBytes() const { return static_cast<size_t>(core->config().memoryLimitMB) * 1024 * 1024 / 8; }

    Error fail(const std::string& u, ErrorCode code, const std::string& msg) {
        emit(ViewEventType::NavigationFailed, msg, code);
        HB_LOG_WARN("navigation to '" << u << "' failed: " << msg);
        return Error{code, msg};
    }

    void saveScroll() { if (index >= 0 && index < static_cast<int>(history.size())) history[static_cast<size_t>(index)].scrollY = page.scrollY(); }

    std::string absoluteHref(const Element* link) const {
        if (!link) return std::string();
        return resolveUrl(url, link->attribute("href"));
    }

    void present(const HistoryEntry& e, const std::string& html, bool restoreScroll) {
        Page::ResourceLoader loader;
        if (e.isFile) {
            std::string baseUrl = stripFragment(e.url);
            size_t limit = maxSourceBytes();
            loader = [baseUrl, limit](const std::string& href) -> std::optional<std::string> {
                if (href.empty()) return std::nullopt;
                std::string abs = resolveUrl(baseUrl, href);
                if (!startsWith(toLowerAscii(abs), "file://")) return std::nullopt;
                std::string content, err;
                if (!readFile(fileUrlToPath(abs), limit, content, err)) return std::nullopt;
                return content;
            };
        }
        page.setSource(html, loader);
        page.setViewport(static_cast<float>(config.width), static_cast<float>(config.height), config.devicePixelRatio);
        downLink = nullptr;
        if (!hoverLink.empty()) { hoverLink.clear(); emit(ViewEventType::HoverLinkChanged, ""); }
        page.documentHeight();
        std::string frag = urlFragment(e.url);
        if (restoreScroll && e.scrollY > 0) page.setScrollY(e.scrollY);
        else if (!frag.empty()) page.scrollToFragment(frag);
        std::string oldTitle = title;
        title = page.title();
        if (url != e.url) { url = e.url; emit(ViewEventType::UrlChanged, url); }
        if (title != oldTitle) emit(ViewEventType::TitleChanged, title);
        emit(ViewEventType::NavigationCompleted, url);
    }

    Error navigateEntry(const HistoryEntry& e, bool rereadFile, bool restoreScroll) {
        emit(ViewEventType::NavigationStarted, e.url);
        std::string html = e.html;
        if (e.isFile && rereadFile) {
            std::string err;
            if (!readFile(e.path, maxSourceBytes(), html, err))
                return fail(e.url, err.find("limit") != std::string::npos ? ErrorCode::InvalidArgument : ErrorCode::IoError, err);
        }
        present(e, html, restoreScroll);
        return Error{};
    }

    Error commit(HistoryEntry e) {
        saveScroll();
        Error err = navigateEntry(e, false, false);
        if (!err.ok()) return err;
        history.resize(static_cast<size_t>(index + 1));
        history.push_back(std::move(e));
        index = static_cast<int>(history.size()) - 1;
        return Error{};
    }

    // Same-document navigation (#fragment): no reparse of the source, but a new history entry.
    Error navigateFragment(const std::string& newUrl) {
        saveScroll();
        emit(ViewEventType::NavigationStarted, newUrl);
        HistoryEntry e = history[static_cast<size_t>(index)];
        e.url = newUrl;
        e.scrollY = 0;
        page.documentHeight();
        std::string frag = urlFragment(newUrl);
        page.scrollToFragment(frag);
        url = newUrl;
        emit(ViewEventType::UrlChanged, url);
        emit(ViewEventType::NavigationCompleted, url);
        history.resize(static_cast<size_t>(index + 1));
        history.push_back(std::move(e));
        index = static_cast<int>(history.size()) - 1;
        return Error{};
    }
};

View::View(Core& core, const ViewConfig& config) : impl_(std::make_unique<Impl>()) {
    impl_->core = &core;
    impl_->config = config;
    impl_->config.width = std::max(1, config.width);
    impl_->config.height = std::max(1, config.height);
    if (!(impl_->config.devicePixelRatio > 0)) impl_->config.devicePixelRatio = 1.0f;
    impl_->page.setViewport(static_cast<float>(impl_->config.width), static_cast<float>(impl_->config.height), impl_->config.devicePixelRatio);
}

View::~View() = default;

Error View::loadHTML(const std::string& html) {
    if (!impl_->core->isInitialized()) return makeErr(ErrorCode::InvalidArgument, "Core is not initialized");
    if (html.size() > impl_->maxSourceBytes()) return impl_->fail("about:blank", ErrorCode::InvalidArgument, "document exceeds the configured memory limit");
    HistoryEntry e;
    e.url = "about:blank";
    e.html = html;
    return impl_->commit(std::move(e));
}

Error View::loadURL(const std::string& rawUrl) {
    if (!impl_->core->isInitialized()) return makeErr(ErrorCode::InvalidArgument, "Core is not initialized");
    std::string url(trim(rawUrl));
    if (url.empty()) return impl_->fail(url, ErrorCode::InvalidArgument, "empty URL");
    std::string lower = toLowerAscii(url);
    HistoryEntry e;
    e.url = url;
    if (lower == "about:blank") return impl_->commit(std::move(e));
    if (startsWith(lower, "data:text/html")) {
        size_t comma = url.find(',');
        if (comma == std::string::npos) return impl_->fail(url, ErrorCode::ParseError, "malformed data: URL");
        std::string head = lower.substr(0, comma);
        if (head.find(";base64") != std::string::npos) {
            bool ok = false;
            e.html = base64Decode(std::string_view(url).substr(comma + 1), &ok);
            if (!ok) return impl_->fail(url, ErrorCode::ParseError, "invalid base64 in data: URL");
        } else {
            e.html = percentDecodeString(url.substr(comma + 1));
        }
        if (e.html.size() > impl_->maxSourceBytes()) return impl_->fail(url, ErrorCode::InvalidArgument, "document exceeds the configured memory limit");
        return impl_->commit(std::move(e));
    }
    if (startsWith(lower, "file://")) {
        // Same document + fragment: scroll instead of reloading.
        if (impl_->index >= 0 && impl_->history[static_cast<size_t>(impl_->index)].isFile &&
            stripFragment(url) == stripFragment(impl_->url) && url.find('#') != std::string::npos)
            return impl_->navigateFragment(url);
        e.isFile = true;
        e.path = fileUrlToPath(url);
        std::string err;
        if (!readFile(e.path, impl_->maxSourceBytes(), e.html, err)) {
            impl_->emit(ViewEventType::NavigationStarted, url);
            return impl_->fail(url, err.find("limit") != std::string::npos ? ErrorCode::InvalidArgument : ErrorCode::NotFound, err);
        }
        if (startsWith(e.html, "\xEF\xBB\xBF")) e.html.erase(0, 3);
        return impl_->commit(std::move(e));
    }
    if (startsWith(lower, "http://") || startsWith(lower, "https://")) {
        impl_->emit(ViewEventType::NavigationStarted, url);
        return impl_->fail(url, ErrorCode::Unsupported, "HB.Network is not implemented yet: http(s) cannot be loaded");
    }
    return impl_->fail(url, ErrorCode::Unsupported, "unsupported URL scheme");
}

Error View::goBack() {
    if (!canGoBack()) return makeErr(ErrorCode::InvalidArgument, "no previous entry");
    impl_->saveScroll();
    int target = impl_->index - 1;
    Error err = impl_->navigateEntry(impl_->history[static_cast<size_t>(target)], false, true);
    if (err.ok()) impl_->index = target;
    return err;
}

Error View::goForward() {
    if (!canGoForward()) return makeErr(ErrorCode::InvalidArgument, "no next entry");
    impl_->saveScroll();
    int target = impl_->index + 1;
    Error err = impl_->navigateEntry(impl_->history[static_cast<size_t>(target)], false, true);
    if (err.ok()) impl_->index = target;
    return err;
}

Error View::reload() {
    if (impl_->index < 0) return makeErr(ErrorCode::InvalidArgument, "nothing to reload");
    impl_->saveScroll();
    return impl_->navigateEntry(impl_->history[static_cast<size_t>(impl_->index)], true, true);
}

bool View::canGoBack() const { return impl_->index > 0; }
bool View::canGoForward() const { return impl_->index >= 0 && impl_->index + 1 < static_cast<int>(impl_->history.size()); }

Error View::executeJavaScript(const std::string&, std::string* result) {
    if (result) result->clear();
    return makeErr(ErrorCode::Unsupported, "HB.JavaScript is not implemented yet");
}

void View::resize(int width, int height) {
    impl_->config.width = std::max(1, width);
    impl_->config.height = std::max(1, height);
    impl_->page.setViewport(static_cast<float>(impl_->config.width), static_cast<float>(impl_->config.height), impl_->config.devicePixelRatio);
}

void View::setDevicePixelRatio(float dpr) {
    if (!(dpr > 0)) return;
    impl_->config.devicePixelRatio = dpr;
    impl_->page.setViewport(static_cast<float>(impl_->config.width), static_cast<float>(impl_->config.height), dpr);
}

void View::scrollBy(float dy) { scrollTo(impl_->page.scrollY() + dy); }

void View::scrollTo(float y) {
    impl_->page.documentHeight();  // make sure layout exists so the clamp is accurate
    impl_->page.setScrollY(y);
}

float View::scrollY() const { return impl_->page.scrollY(); }
float View::documentHeight() { return impl_->page.documentHeight(); }
std::string View::title() const { return impl_->title; }
std::string View::url() const { return impl_->url; }

// ---- Input ----

void View::mouseMove(float x, float y) {
    HitTestResult hit = impl_->page.hitTest(x, y);
    std::string link = impl_->absoluteHref(hit.link);
    if (link != impl_->hoverLink) {
        impl_->hoverLink = link;
        impl_->emit(ViewEventType::HoverLinkChanged, link);
    }
    CursorType c = hit.link ? CursorType::Hand : (hit.overText ? CursorType::Text : CursorType::Arrow);
    if (c != impl_->cursor) {
        impl_->cursor = c;
        impl_->emit(ViewEventType::CursorChanged, "");
    }
}

void View::mouseDown(float x, float y, MouseButton button) {
    HitTestResult hit = impl_->page.hitTest(x, y);
    impl_->downLink = hit.link;
    impl_->downButton = button;
}

void View::mouseUp(float x, float y, MouseButton button, bool ctrl) {
    HitTestResult hit = impl_->page.hitTest(x, y);
    const Element* down = impl_->downLink;
    MouseButton downButton = impl_->downButton;
    impl_->downLink = nullptr;
    if (button == MouseButton::Right) {
        impl_->emit(ViewEventType::ContextMenuRequested, impl_->absoluteHref(hit.link), ErrorCode::Ok, x, y);
        return;
    }
    if (!hit.link || hit.link != down || button != downButton) return;
    std::string target = impl_->absoluteHref(hit.link);
    if (target.empty()) return;  // relative link on a document without a base URL
    std::string targetAttr = toLowerAscii(hit.link->attribute("target"));
    if (button == MouseButton::Middle || ctrl || targetAttr == "_blank") {
        impl_->emit(ViewEventType::NewWindowRequested, target);
        return;
    }
    if (impl_->index >= 0 && target.find('#') != std::string::npos &&
        stripFragment(target) == stripFragment(impl_->url) && !startsWith(toLowerAscii(target), "data:")) {
        impl_->navigateFragment(target);
        return;
    }
    loadURL(target);
}

void View::mouseWheel(float deltaY) { scrollBy(deltaY); }

void View::keyDown(Key key, bool shift) {
    if (!impl_->focused) return;
    float page = impl_->config.height * 0.875f;
    switch (key) {
        case Key::Up: scrollBy(-40); break;
        case Key::Down: scrollBy(40); break;
        case Key::PageUp: scrollBy(-page); break;
        case Key::PageDown: scrollBy(page); break;
        case Key::Space: scrollBy(shift ? -page : page); break;
        case Key::Home: scrollTo(0); break;
        case Key::End: scrollTo(documentHeight()); break;
    }
}

void View::setFocus(bool focused) { impl_->focused = focused; }
bool View::hasFocus() const { return impl_->focused; }
CursorType View::cursor() const { return impl_->cursor; }

std::string View::linkAt(float x, float y) {
    return impl_->absoluteHref(impl_->page.hitTest(x, y).link);
}

Framebuffer View::render() {
    const Surface& s = impl_->page.render();
    return Framebuffer{s.width(), s.height(), s.stride(), s.data()};
}

Error View::savePNG(const std::string& path) {
    Framebuffer fb = render();
    std::vector<uint8_t> png = encodePng(fb.pixels, fb.width, fb.height, fb.strideBytes);
    if (png.empty()) return makeErr(ErrorCode::Internal, "PNG encoding failed");
    std::ofstream f(path, std::ios::binary);
    if (!f) return makeErr(ErrorCode::IoError, "cannot write file: " + path);
    f.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!f) return makeErr(ErrorCode::IoError, "write failed: " + path);
    return Error{};
}

void View::setEventHandler(std::function<void(const ViewEvent&)> handler) { impl_->handler = std::move(handler); }

} // namespace HB
