// HB Core - Win32 host helpers. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
// Licensed under the Homebax Studios Non-Commercial License (see LICENSE).
//
// STATUS: written for Windows 7+ (GDI only, no Windows 8+/10-only API) but NOT compiled or tested yet
// (the 0.2.0 build environment was Linux). Expect small fixes on the first Windows build.
//
// Uses only the stable C ABI (hbcore/hb_core_c.h). Typical use from a window procedure:
//     case WM_CREATE: g_view = hb_view_create(core, &cfg); hb_win32_attach(g_view, hwnd); break;
//     default: { LRESULT r; if (hb_win32_handle_message(g_view, hwnd, msg, wp, lp, &r)) return r; }
#ifndef HBCORE_HB_WIN32_H
#define HBCORE_HB_WIN32_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <hbcore/hb_core_c.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sets size and device pixel ratio of the view from the window's client area and DPI. */
void hb_win32_attach(HB_View* view, HWND hwnd);

/* Renders the view and copies it to the device context at (x, y). Returns non-zero on success. */
int hb_win32_present(HB_View* view, HDC hdc, int x, int y);

/* Translates WM_SIZE, WM_PAINT, WM_ERASEBKGND, mouse, wheel, keyboard, focus and WM_SETCURSOR messages into
 * HB Core input calls. Returns non-zero if the message was handled; *result then holds the return value. */
int hb_win32_handle_message(HB_View* view, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result);

#ifdef __cplusplus
}
#endif
#endif
