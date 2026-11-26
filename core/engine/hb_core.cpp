// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "hbcore/hb_core.h"
#include "base/log.hpp"
#include "base/strings.hpp"
#include "engine/page.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace HB {

namespace {

ErrorCode toPublic(::HB::ErrorCode c) { return c; }

std::string percentDecode(std::string_view s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() + 0 && std::isxdigit(static_cast<unsigned char>(s[i + 1])) && std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
            out.push_back(static_cast<char>(std::stoi(std::string(s.substr(i + 1, 2)), nullptr, 16)));
            i += 2;
        } else out.push_back(s[i]);
    }
    return out;
}

// Returns a filesystem path for file:// URLs.
std::string fileUrlToPath(const std::string& url) {
    std::string p = percentDecode(url.substr(7));  // after "file://"
    if (p.size() >= 3 && p[0] == '/' && isAsciiAlpha(p[1]) && p[2] == ':') p.erase(0, 1);  // /C:/x -> C:/x
    return p;
}

std::string dirOf(const std::string& path) {
    size_t s = path.find_last_of("/\\");
    return s == std::string::npos ? std::string() : path.substr(0, s + 1);
}

bool readFile(const std::string& path, size_t maxBytes, std::string& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { err = "cannot open file: " + path; return false; }
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    if (out.size() > maxBytes) { err = "document exceeds the configured memory limit"; return false; }
    if (startsWith(out, "\xEF\xBB\xBF")) out.erase(0, 3);  // UTF-8 BOM
    return true;
}

Error makeErr(::HB::ErrorCode c, std::string m) { return Error{toPublic(c), std::move(m)}; }

} // namespace

// ---- Core ----

struct Core::Impl {
    CoreConfig config;
    bool initialized = false;
};

Core::Core() : impl_(std::make_unique<Impl>()) {}
Core::~Core() { shutdown(); }

Error Core::initialize(const CoreConfig& config) {
    if (impl_->initialized) return makeErr(::HB::ErrorCode::InvalidArgument, "Core is already initialized");
    if (config.memoryLimitMB == 0) return makeErr(::HB::ErrorCode::InvalidArgument, "memoryLimitMB must be > 0");
    impl_->config = config;
    int lvl = std::clamp(config.logLevel, 0, 5);
    Logger::instance().setLevel(static_cast<LogLevel>(lvl));
    impl_->initialized = true;
    HB_LOG_INFO("HB Core " << HBCORE_VERSION_STRING << " initialized (CPU renderer)");
    return Error{};
}

bool Core::isInitialized() const { return impl_->initialized; }
const CoreConfig& Core::config() const { return impl_->config; }
void Core::shutdown() { impl_->initialized = false; }
const char* Core::version() { return HBCORE_VERSION_STRING; }

// ---- View ----

struct HistoryEntry {
    std::string url;
    std::string html;      // snapshot for non-file entries
    bool isFile = false;
    std::string path;
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

    void emit(ViewEventType t, const std::string& text, ErrorCode c = ErrorCode::Ok) {
        if (handler) handler(ViewEvent{t, text, c});
    }

    size_t maxSourceBytes() const { return static_cast<size_t>(core->config().memoryLimitMB) * 1024 * 1024 / 8; }

    Error fail(const std::string& url, ErrorCode code, const std::string& msg) {
        emit(ViewEventType::NavigationFailed, msg, code);
        HB_LOG_WARN("navigation to '" << url << "' failed: " << msg);
        return Error{code, msg};
    }

    void present(const HistoryEntry& e, const std::string& html) {
        Page::ResourceLoader loader;
        if (e.isFile) {
            std::string base = dirOf(e.path);
            size_t limit = maxSourceBytes();
            loader = [base, limit](const std::string& href) -> std::optional<std::string> {
                if (href.empty() || href.find("://") != std::string::npos || startsWith(href, "data:")) return std::nullopt;
                std::string content, err;
                if (!readFile(base + percentDecode(href), limit, content, err)) return std::nullopt;
                return content;
            };
        }
        page.setSource(html, loader);
        page.setViewport(static_cast<float>(config.width), static_cast<float>(config.height), config.devicePixelRatio);
        std::string oldTitle = title;
        title = page.title();
        if (url != e.url) { url = e.url; emit(ViewEventType::UrlChanged, url); }
        if (title != oldTitle) emit(ViewEventType::TitleChanged, title);
        emit(ViewEventType::NavigationCompleted, url);
    }

    Error navigateEntry(const HistoryEntry& e, bool rereadFile) {
        emit(ViewEventType::NavigationStarted, e.url);
        std::string html = e.html;
        if (e.isFile && rereadFile) {
            std::string err;
            if (!readFile(e.path, maxSourceBytes(), html, err))
                return fail(e.url, err.find("limit") != std::string::npos ? ErrorCode::InvalidArgument : ErrorCode::IoError, err);
        }
        present(e, html);
        return Error{};
    }

    Error commit(HistoryEntry e) {
        Error err = navigateEntry(e, false);
        if (!err.ok()) return err;
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
    if (!impl_->core->isInitialized()) return makeErr(::HB::ErrorCode::InvalidArgument, "Core is not initialized");
    if (html.size() > impl_->maxSourceBytes()) return impl_->fail("about:blank", ErrorCode::InvalidArgument, "document exceeds the configured memory limit");
    HistoryEntry e;
    e.url = "about:blank";
    e.html = html;
    return impl_->commit(std::move(e));
}

Error View::loadURL(const std::string& rawUrl) {
    if (!impl_->core->isInitialized()) return makeErr(::HB::ErrorCode::InvalidArgument, "Core is not initialized");
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
        if (head.find(";base64") != std::string::npos) return impl_->fail(url, ErrorCode::Unsupported, "base64 data: URLs are not supported in 0.1.0");
        e.html = percentDecode(std::string_view(url).substr(comma + 1));
        if (e.html.size() > impl_->maxSourceBytes()) return impl_->fail(url, ErrorCode::InvalidArgument, "document exceeds the configured memory limit");
        return impl_->commit(std::move(e));
    }
    if (startsWith(lower, "file://")) {
        e.isFile = true;
        e.path = fileUrlToPath(url);
        std::string err;
        if (!readFile(e.path, impl_->maxSourceBytes(), e.html, err)) {
            impl_->emit(ViewEventType::NavigationStarted, url);
            return impl_->fail(url, err.find("limit") != std::string::npos ? ErrorCode::InvalidArgument : ErrorCode::NotFound, err);
        }
        return impl_->commit(std::move(e));
    }
    if (startsWith(lower, "http://") || startsWith(lower, "https://")) {
        impl_->emit(ViewEventType::NavigationStarted, url);
        return impl_->fail(url, ErrorCode::Unsupported, "HB.Network is not implemented in 0.1.0: http(s) cannot be loaded");
    }
    return impl_->fail(url, ErrorCode::Unsupported, "unsupported URL scheme");
}

Error View::goBack() {
    if (!canGoBack()) return makeErr(::HB::ErrorCode::InvalidArgument, "no previous entry");
    int target = impl_->index - 1;
    Error err = impl_->navigateEntry(impl_->history[static_cast<size_t>(target)], false);
    if (err.ok()) impl_->index = target;
    return err;
}

Error View::goForward() {
    if (!canGoForward()) return makeErr(::HB::ErrorCode::InvalidArgument, "no next entry");
    int target = impl_->index + 1;
    Error err = impl_->navigateEntry(impl_->history[static_cast<size_t>(target)], false);
    if (err.ok()) impl_->index = target;
    return err;
}

Error View::reload() {
    if (impl_->index < 0) return makeErr(::HB::ErrorCode::InvalidArgument, "nothing to reload");
    return impl_->navigateEntry(impl_->history[static_cast<size_t>(impl_->index)], true);
}

bool View::canGoBack() const { return impl_->index > 0; }
bool View::canGoForward() const { return impl_->index >= 0 && impl_->index + 1 < static_cast<int>(impl_->history.size()); }

Error View::executeJavaScript(const std::string&, std::string* result) {
    if (result) result->clear();
    return makeErr(::HB::ErrorCode::Unsupported, "HB.JavaScript is not implemented in 0.1.0");
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

Framebuffer View::render() {
    const Surface& s = impl_->page.render();
    return Framebuffer{s.width(), s.height(), s.stride(), s.data()};
}

Error View::savePNG(const std::string& path) {
    Framebuffer fb = render();
    std::vector<uint8_t> png = encodePng(fb.pixels, fb.width, fb.height, fb.strideBytes);
    if (png.empty()) return makeErr(::HB::ErrorCode::Internal, "PNG encoding failed");
    std::ofstream f(path, std::ios::binary);
    if (!f) return makeErr(::HB::ErrorCode::IoError, "cannot write file: " + path);
    f.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!f) return makeErr(::HB::ErrorCode::IoError, "write failed: " + path);
    return Error{};
}

void View::setEventHandler(std::function<void(const ViewEvent&)> handler) { impl_->handler = std::move(handler); }

} // namespace HB
