/* HB Core - stable C ABI. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
 * Licensed under the Homebax Studios Non-Commercial License (see LICENSE).
 *
 * All handles are opaque. No internal C++ object is exposed.
 * Strings are UTF-8, NUL-terminated. Pointers returned by hb_view_* getters stay valid
 * until the next call on the same view that changes it (or until the view is destroyed).
 */
#ifndef HBCORE_HB_CORE_C_H
#define HBCORE_HB_CORE_C_H

#include <stddef.h>
#include <stdint.h>
#include "hb_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HB_ABI_VERSION 1

typedef struct HB_Core HB_Core;
typedef struct HB_View HB_View;

typedef enum HB_Result {
    HB_OK = 0,
    HB_ERROR_INVALID_ARGUMENT = 1,
    HB_ERROR_NOT_FOUND = 2,
    HB_ERROR_IO = 3,
    HB_ERROR_PARSE = 4,
    HB_ERROR_UNSUPPORTED = 5,
    HB_ERROR_INTERNAL = 6
} HB_Result;

typedef struct HB_CoreConfig {
    uint32_t struct_size;        /* set to sizeof(HB_CoreConfig) */
    int32_t  enable_javascript;  /* accepted; no JS engine exists yet */
    int32_t  enable_gpu;         /* accepted; only the CPU renderer exists yet */
    uint32_t memory_limit_mb;    /* 0 = default (512) */
    int32_t  log_level;          /* 0 trace .. 4 error, 5 off; default 2 (info) */
    /* Added in 0.2.0 (additive: older callers with a smaller struct_size keep the defaults). */
    int32_t  disable_system_fonts; /* 0 (default) = discover TrueType system fonts */
    const char* font_directory;  /* optional extra font directory, may be NULL */
} HB_CoreConfig;

typedef struct HB_ViewConfig {
    uint32_t struct_size;        /* set to sizeof(HB_ViewConfig) */
    int32_t  width;              /* CSS pixels, default 800 */
    int32_t  height;             /* CSS pixels, default 600 */
    float    device_pixel_ratio; /* default 1.0 */
} HB_ViewConfig;

typedef enum HB_EventType {
    HB_EVENT_NAVIGATION_STARTED = 1,
    HB_EVENT_NAVIGATION_COMPLETED = 2,
    HB_EVENT_NAVIGATION_FAILED = 3,
    HB_EVENT_TITLE_CHANGED = 4,
    HB_EVENT_URL_CHANGED = 5,
    HB_EVENT_ERROR = 6,
    HB_EVENT_HOVER_LINK_CHANGED = 7,    /* text = absolute URL under the mouse, "" when none */
    HB_EVENT_CURSOR_CHANGED = 8,        /* query hb_view_get_cursor() */
    HB_EVENT_CONTEXT_MENU_REQUESTED = 9,/* text = link URL (may be ""), x/y = pointer position */
    HB_EVENT_NEW_WINDOW_REQUESTED = 10  /* text = absolute URL; the host decides what to do */
} HB_EventType;

typedef struct HB_Event {
    HB_EventType type;
    const char*  text;   /* url, title or message depending on type; valid during the callback */
    int32_t      code;   /* HB_Result for failures, 0 otherwise */
    float        x, y;   /* context menu position (viewport CSS px), 0 otherwise */
} HB_Event;

typedef enum HB_MouseButton { HB_MOUSE_LEFT = 0, HB_MOUSE_MIDDLE = 1, HB_MOUSE_RIGHT = 2 } HB_MouseButton;
typedef enum HB_Key { HB_KEY_UP = 0, HB_KEY_DOWN = 1, HB_KEY_PAGE_UP = 2, HB_KEY_PAGE_DOWN = 3, HB_KEY_HOME = 4, HB_KEY_END = 5, HB_KEY_SPACE = 6 } HB_Key;
typedef enum HB_Cursor { HB_CURSOR_ARROW = 0, HB_CURSOR_HAND = 1, HB_CURSOR_TEXT = 2 } HB_Cursor;

typedef void (*HB_EventCallback)(const HB_Event* event, void* user_data);

HB_API const char* hb_version_string(void);
HB_API int hb_abi_version(void);
HB_API const char* hb_result_string(HB_Result r);

HB_API HB_Core* hb_core_create(const HB_CoreConfig* config);
HB_API void hb_core_destroy(HB_Core* core);

HB_API HB_View* hb_view_create(HB_Core* core, const HB_ViewConfig* config);
HB_API void hb_view_destroy(HB_View* view);

HB_API HB_Result hb_view_load_url(HB_View* view, const char* url);
HB_API HB_Result hb_view_load_html(HB_View* view, const char* html);
HB_API HB_Result hb_view_go_back(HB_View* view);
HB_API HB_Result hb_view_go_forward(HB_View* view);
HB_API HB_Result hb_view_reload(HB_View* view);
HB_API HB_Result hb_view_execute_javascript(HB_View* view, const char* script); /* HB_ERROR_UNSUPPORTED in 0.1.0 */

HB_API void hb_view_resize(HB_View* view, int width, int height);
HB_API void hb_view_set_device_pixel_ratio(HB_View* view, float dpr);
HB_API void hb_view_scroll_by(HB_View* view, float dy);

HB_API const char* hb_view_get_title(HB_View* view);
HB_API const char* hb_view_get_url(HB_View* view);
HB_API float hb_view_get_document_height(HB_View* view);

/* Renders the current page into an internal RGBA8 (non-premultiplied, opaque) framebuffer. */
HB_API HB_Result hb_view_render(HB_View* view);
HB_API const uint8_t* hb_view_get_pixels(HB_View* view, int* width, int* height, int* stride_bytes);
HB_API HB_Result hb_view_save_png(HB_View* view, const char* path);

/* Input. Coordinates are viewport CSS pixels. */
HB_API void hb_view_mouse_move(HB_View* view, float x, float y);
HB_API void hb_view_mouse_down(HB_View* view, float x, float y, HB_MouseButton button);
HB_API void hb_view_mouse_up(HB_View* view, float x, float y, HB_MouseButton button, int ctrl);
HB_API void hb_view_mouse_wheel(HB_View* view, float delta_y);   /* CSS px, positive = scroll down */
HB_API void hb_view_key_down(HB_View* view, HB_Key key, int shift);
HB_API void hb_view_set_focus(HB_View* view, int focused);
HB_API HB_Cursor hb_view_get_cursor(HB_View* view);
HB_API const char* hb_view_get_link_at(HB_View* view, float x, float y); /* absolute URL or "" */
HB_API float hb_view_get_scroll_y(HB_View* view);

/* Registers a TrueType font file. family: "sans", "serif" or "mono". */
HB_API HB_Result hb_core_load_font(HB_Core* core, const char* family, int bold, int italic, const char* path);

HB_API void hb_view_set_event_callback(HB_View* view, HB_EventCallback cb, void* user_data);

#ifdef __cplusplus
}
#endif
#endif
