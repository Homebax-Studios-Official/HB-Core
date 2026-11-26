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
    int32_t  enable_javascript;  /* accepted; no JS engine exists in 0.1.0 */
    int32_t  enable_gpu;         /* accepted; only the CPU renderer exists in 0.1.0 */
    uint32_t memory_limit_mb;    /* 0 = default (512) */
    int32_t  log_level;          /* 0 trace .. 4 error, 5 off; default 2 (info) */
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
    HB_EVENT_ERROR = 6
} HB_EventType;

typedef struct HB_Event {
    HB_EventType type;
    const char*  text;   /* url, title or message depending on type; valid during the callback */
    int32_t      code;   /* HB_Result for failures, 0 otherwise */
} HB_Event;

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

HB_API void hb_view_set_event_callback(HB_View* view, HB_EventCallback cb, void* user_data);

#ifdef __cplusplus
}
#endif
#endif
