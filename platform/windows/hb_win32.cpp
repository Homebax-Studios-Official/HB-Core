// HB Core - Win32 host helpers. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
// STATUS: not compiled or tested yet (see hb_win32.h).
#include "hb_win32.h"
#include <windowsx.h>
#include <vector>

namespace {

float windowDpr(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;  // system DPI; works on Windows 7+
    if (dc) ReleaseDC(hwnd, dc);
    return dpi > 0 ? static_cast<float>(dpi) / 96.0f : 1.0f;
}

float g_dpr = 1.0f;  // single-window helper: last DPR applied by hb_win32_attach / WM_SIZE

}  // namespace

extern "C" {

void hb_win32_attach(HB_View* view, HWND hwnd) {
    if (!view || !hwnd) return;
    g_dpr = windowDpr(hwnd);
    RECT rc;
    GetClientRect(hwnd, &rc);
    hb_view_set_device_pixel_ratio(view, g_dpr);
    hb_view_resize(view, static_cast<int>((rc.right - rc.left) / g_dpr), static_cast<int>((rc.bottom - rc.top) / g_dpr));
}

int hb_win32_present(HB_View* view, HDC hdc, int x, int y) {
    if (!view || !hdc) return 0;
    int w = 0, h = 0, stride = 0;
    const uint8_t* rgba = hb_view_get_pixels(view, &w, &h, &stride);
    if (!rgba || w <= 0 || h <= 0) return 0;
    std::vector<uint8_t> bgra(static_cast<size_t>(w) * h * 4);
    for (int row = 0; row < h; ++row) {
        const uint8_t* s = rgba + static_cast<size_t>(row) * stride;
        uint8_t* d = &bgra[static_cast<size_t>(row) * w * 4];
        for (int col = 0; col < w; ++col) { d[col * 4] = s[col * 4 + 2]; d[col * 4 + 1] = s[col * 4 + 1]; d[col * 4 + 2] = s[col * 4]; d[col * 4 + 3] = 255; }
    }
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;  // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    return SetDIBitsToDevice(hdc, x, y, static_cast<DWORD>(w), static_cast<DWORD>(h), 0, 0, 0, static_cast<UINT>(h), bgra.data(), &bi, DIB_RGB_COLORS) != 0;
}

int hb_win32_handle_message(HB_View* view, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result) {
    if (!view) return 0;
    LRESULT r = 0;
    auto cx = [&](LPARAM lp) { return static_cast<float>(GET_X_LPARAM(lp)) / g_dpr; };
    auto cy = [&](LPARAM lp) { return static_cast<float>(GET_Y_LPARAM(lp)) / g_dpr; };
    switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                hb_view_resize(view, static_cast<int>(LOWORD(lParam) / g_dpr), static_cast<int>(HIWORD(lParam) / g_dpr));
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        case WM_ERASEBKGND:
            r = 1;  // we paint the whole client area
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            hb_win32_present(view, dc, 0, 0);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_MOUSEMOVE:
            hb_view_mouse_move(view, cx(lParam), cy(lParam));
            break;
        case WM_LBUTTONDOWN: SetCapture(hwnd); hb_view_mouse_down(view, cx(lParam), cy(lParam), HB_MOUSE_LEFT); break;
        case WM_MBUTTONDOWN: SetCapture(hwnd); hb_view_mouse_down(view, cx(lParam), cy(lParam), HB_MOUSE_MIDDLE); break;
        case WM_RBUTTONDOWN: SetCapture(hwnd); hb_view_mouse_down(view, cx(lParam), cy(lParam), HB_MOUSE_RIGHT); break;
        case WM_LBUTTONUP: ReleaseCapture(); hb_view_mouse_up(view, cx(lParam), cy(lParam), HB_MOUSE_LEFT, (GetKeyState(VK_CONTROL) & 0x8000) != 0); InvalidateRect(hwnd, nullptr, FALSE); break;
        case WM_MBUTTONUP: ReleaseCapture(); hb_view_mouse_up(view, cx(lParam), cy(lParam), HB_MOUSE_MIDDLE, 0); break;
        case WM_RBUTTONUP: ReleaseCapture(); hb_view_mouse_up(view, cx(lParam), cy(lParam), HB_MOUSE_RIGHT, 0); break;
        case WM_MOUSEWHEEL: {
            int lines = GET_WHEEL_DELTA_WPARAM(wParam);
            hb_view_mouse_wheel(view, -static_cast<float>(lines) / WHEEL_DELTA * 48.0f);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        case WM_KEYDOWN: {
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            int key = -1;
            switch (wParam) {
                case VK_UP: key = HB_KEY_UP; break;
                case VK_DOWN: key = HB_KEY_DOWN; break;
                case VK_PRIOR: key = HB_KEY_PAGE_UP; break;
                case VK_NEXT: key = HB_KEY_PAGE_DOWN; break;
                case VK_HOME: key = HB_KEY_HOME; break;
                case VK_END: key = HB_KEY_END; break;
                case VK_SPACE: key = HB_KEY_SPACE; break;
                default: break;
            }
            if (key < 0) return 0;
            hb_view_key_down(view, static_cast<HB_Key>(key), shift ? 1 : 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        case WM_SETFOCUS: hb_view_set_focus(view, 1); break;
        case WM_KILLFOCUS: hb_view_set_focus(view, 0); break;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                LPCWSTR id = IDC_ARROW;
                switch (hb_view_get_cursor(view)) {
                    case HB_CURSOR_HAND: id = IDC_HAND; break;
                    case HB_CURSOR_TEXT: id = IDC_IBEAM; break;
                    default: break;
                }
                SetCursor(LoadCursorW(nullptr, id));
                r = TRUE;
                break;
            }
            return 0;
        default:
            return 0;
    }
    if (result) *result = r;
    return 1;
}

}  // extern "C"
