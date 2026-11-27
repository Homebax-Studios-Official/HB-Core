// HB Core - C ABI implementation. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "hbcore/hb_core_c.h"
#include "hbcore/hb_core.h"
#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <string>

struct HB_Core {
    HB::Core core;
};

struct HB_View {
    HB_Core* owner;
    std::unique_ptr<HB::View> view;
    std::string title;
    std::string url;
    HB_EventCallback callback = nullptr;
    void* userData = nullptr;
};

namespace {

HB_Result convert(HB::ErrorCode c) {
    switch (c) {
        case HB::ErrorCode::Ok: return HB_OK;
        case HB::ErrorCode::InvalidArgument: return HB_ERROR_INVALID_ARGUMENT;
        case HB::ErrorCode::NotFound: return HB_ERROR_NOT_FOUND;
        case HB::ErrorCode::IoError: return HB_ERROR_IO;
        case HB::ErrorCode::ParseError: return HB_ERROR_PARSE;
        case HB::ErrorCode::Unsupported: return HB_ERROR_UNSUPPORTED;
        default: return HB_ERROR_INTERNAL;
    }
}

HB_Result convert(const HB::Error& e) { return convert(e.code); }

HB_EventType convertType(HB::ViewEventType t) {
    switch (t) {
        case HB::ViewEventType::NavigationStarted: return HB_EVENT_NAVIGATION_STARTED;
        case HB::ViewEventType::NavigationCompleted: return HB_EVENT_NAVIGATION_COMPLETED;
        case HB::ViewEventType::NavigationFailed: return HB_EVENT_NAVIGATION_FAILED;
        case HB::ViewEventType::TitleChanged: return HB_EVENT_TITLE_CHANGED;
        case HB::ViewEventType::UrlChanged: return HB_EVENT_URL_CHANGED;
        case HB::ViewEventType::HoverLinkChanged: return HB_EVENT_HOVER_LINK_CHANGED;
        case HB::ViewEventType::CursorChanged: return HB_EVENT_CURSOR_CHANGED;
        case HB::ViewEventType::ContextMenuRequested: return HB_EVENT_CONTEXT_MENU_REQUESTED;
        case HB::ViewEventType::NewWindowRequested: return HB_EVENT_NEW_WINDOW_REQUESTED;
        default: return HB_EVENT_ERROR;
    }
}

// The C ABI never lets a C++ exception cross the boundary.
template <class F>
HB_Result guarded(F&& f) {
    try { return f(); }
    catch (const std::bad_alloc&) { return HB_ERROR_INTERNAL; }
    catch (...) { return HB_ERROR_INTERNAL; }
}

} // namespace

extern "C" {

const char* hb_version_string(void) { return HBCORE_VERSION_STRING; }
int hb_abi_version(void) { return HB_ABI_VERSION; }

const char* hb_result_string(HB_Result r) {
    switch (r) {
        case HB_OK: return "ok";
        case HB_ERROR_INVALID_ARGUMENT: return "invalid argument";
        case HB_ERROR_NOT_FOUND: return "not found";
        case HB_ERROR_IO: return "i/o error";
        case HB_ERROR_PARSE: return "parse error";
        case HB_ERROR_UNSUPPORTED: return "unsupported";
        case HB_ERROR_INTERNAL: return "internal error";
    }
    return "unknown";
}

HB_Core* hb_core_create(const HB_CoreConfig* config) {
    try {
        HB::CoreConfig cfg;
        // struct_size lets old callers (smaller struct) keep working: only read fields that fit.
        if (config && config->struct_size >= offsetof(HB_CoreConfig, log_level) + sizeof(int32_t)) {
            cfg.enableJavaScript = config->enable_javascript != 0;
            cfg.enableGPU = config->enable_gpu != 0;
            if (config->memory_limit_mb) cfg.memoryLimitMB = config->memory_limit_mb;
            if (config->log_level >= 0 && config->log_level <= 5) cfg.logLevel = config->log_level;
            if (config->struct_size >= offsetof(HB_CoreConfig, disable_system_fonts) + sizeof(int32_t))
                cfg.useSystemFonts = config->disable_system_fonts == 0;
            if (config->struct_size >= offsetof(HB_CoreConfig, font_directory) + sizeof(const char*) && config->font_directory)
                cfg.fontDirectory = config->font_directory;
        }
        std::unique_ptr<HB_Core> c(new HB_Core());
        if (!c->core.initialize(cfg).ok()) return nullptr;
        return c.release();
    } catch (...) {
        return nullptr;
    }
}

void hb_core_destroy(HB_Core* core) { delete core; }

HB_Result hb_core_load_font(HB_Core* core, const char* family, int bold, int italic, const char* path) {
    if (!core || !family || !path) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return core->core.loadFont(family, bold != 0, italic != 0, path) ? HB_OK : HB_ERROR_INVALID_ARGUMENT; });
}

HB_View* hb_view_create(HB_Core* core, const HB_ViewConfig* config) {
    if (!core) return nullptr;
    try {
        HB::ViewConfig cfg;
        if (config && config->struct_size >= sizeof(uint32_t) + 2 * sizeof(int32_t)) {
            if (config->width > 0) cfg.width = config->width;
            if (config->height > 0) cfg.height = config->height;
            if (config->struct_size >= sizeof(HB_ViewConfig) && config->device_pixel_ratio > 0) cfg.devicePixelRatio = config->device_pixel_ratio;
        }
        std::unique_ptr<HB_View> v(new HB_View());
        v->owner = core;
        v->view = std::make_unique<HB::View>(core->core, cfg);
        HB_View* raw = v.get();
        raw->view->setEventHandler([raw](const HB::ViewEvent& ev) {
            if (!raw->callback) return;
            HB_Event e;
            e.type = convertType(ev.type);
            e.text = ev.text.c_str();
            e.code = static_cast<int32_t>(convert(ev.code));
            e.x = ev.x;
            e.y = ev.y;
            raw->callback(&e, raw->userData);
        });
        return v.release();
    } catch (...) {
        return nullptr;
    }
}

void hb_view_destroy(HB_View* view) { delete view; }

HB_Result hb_view_load_url(HB_View* view, const char* url) {
    if (!view || !url) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->loadURL(url)); });
}

HB_Result hb_view_load_html(HB_View* view, const char* html) {
    if (!view || !html) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->loadHTML(html)); });
}

HB_Result hb_view_go_back(HB_View* view) {
    if (!view) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->goBack()); });
}

HB_Result hb_view_go_forward(HB_View* view) {
    if (!view) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->goForward()); });
}

HB_Result hb_view_reload(HB_View* view) {
    if (!view) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->reload()); });
}

HB_Result hb_view_execute_javascript(HB_View* view, const char* script) {
    if (!view || !script) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->executeJavaScript(script)); });
}

void hb_view_resize(HB_View* view, int width, int height) {
    if (!view) return;
    try { view->view->resize(width, height); } catch (...) {}
}

void hb_view_set_device_pixel_ratio(HB_View* view, float dpr) {
    if (!view) return;
    try { view->view->setDevicePixelRatio(dpr); } catch (...) {}
}

void hb_view_scroll_by(HB_View* view, float dy) {
    if (!view) return;
    try { view->view->scrollBy(dy); } catch (...) {}
}

const char* hb_view_get_title(HB_View* view) {
    if (!view) return "";
    try { view->title = view->view->title(); return view->title.c_str(); } catch (...) { return ""; }
}

const char* hb_view_get_url(HB_View* view) {
    if (!view) return "";
    try { view->url = view->view->url(); return view->url.c_str(); } catch (...) { return ""; }
}

float hb_view_get_document_height(HB_View* view) {
    if (!view) return 0;
    try { return view->view->documentHeight(); } catch (...) { return 0; }
}

HB_Result hb_view_render(HB_View* view) {
    if (!view) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { view->view->render(); return HB_OK; });
}

const uint8_t* hb_view_get_pixels(HB_View* view, int* width, int* height, int* stride_bytes) {
    if (!view) return nullptr;
    try {
        HB::Framebuffer fb = view->view->render();
        if (width) *width = fb.width;
        if (height) *height = fb.height;
        if (stride_bytes) *stride_bytes = fb.strideBytes;
        return fb.pixels;
    } catch (...) {
        return nullptr;
    }
}

HB_Result hb_view_save_png(HB_View* view, const char* path) {
    if (!view || !path) return HB_ERROR_INVALID_ARGUMENT;
    return guarded([&] { return convert(view->view->savePNG(path)); });
}

void hb_view_mouse_move(HB_View* view, float x, float y) { if (view) { try { view->view->mouseMove(x, y); } catch (...) {} } }
void hb_view_mouse_down(HB_View* view, float x, float y, HB_MouseButton b) {
    if (view) { try { view->view->mouseDown(x, y, static_cast<HB::MouseButton>(b)); } catch (...) {} }
}
void hb_view_mouse_up(HB_View* view, float x, float y, HB_MouseButton b, int ctrl) {
    if (view) { try { view->view->mouseUp(x, y, static_cast<HB::MouseButton>(b), ctrl != 0); } catch (...) {} }
}
void hb_view_mouse_wheel(HB_View* view, float dy) { if (view) { try { view->view->mouseWheel(dy); } catch (...) {} } }
void hb_view_key_down(HB_View* view, HB_Key key, int shift) {
    if (view) { try { view->view->keyDown(static_cast<HB::Key>(key), shift != 0); } catch (...) {} }
}
void hb_view_set_focus(HB_View* view, int focused) { if (view) view->view->setFocus(focused != 0); }
HB_Cursor hb_view_get_cursor(HB_View* view) {
    if (!view) return HB_CURSOR_ARROW;
    return static_cast<HB_Cursor>(view->view->cursor());
}
const char* hb_view_get_link_at(HB_View* view, float x, float y) {
    if (!view) return "";
    try { view->url = view->view->linkAt(x, y); return view->url.c_str(); } catch (...) { return ""; }
}
float hb_view_get_scroll_y(HB_View* view) { return view ? view->view->scrollY() : 0.0f; }

void hb_view_set_event_callback(HB_View* view, HB_EventCallback cb, void* user_data) {
    if (!view) return;
    view->callback = cb;
    view->userData = user_data;
}

} // extern "C"
