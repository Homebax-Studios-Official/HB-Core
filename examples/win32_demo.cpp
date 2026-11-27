// HB Core Win32 demo: a window that displays a page and handles input through hb_win32_*.
// STATUS: not compiled or tested yet (written on Linux). Usage: hb_win32_demo.exe [file-or-url]
#include <hbcore/hb_core_c.h>
#include "hb_win32.h"
#include <string>

static HB_Core* g_core = nullptr;
static HB_View* g_view = nullptr;

static void onEvent(const HB_Event* e, void* user) {
    HWND hwnd = static_cast<HWND>(user);
    if (e->type == HB_EVENT_TITLE_CHANGED) {
        int n = MultiByteToWideChar(CP_UTF8, 0, e->text, -1, nullptr, 0);
        std::wstring w(static_cast<size_t>(n > 0 ? n : 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, e->text, -1, &w[0], n);
        SetWindowTextW(hwnd, (L"HB Core - " + w).c_str());
    }
    if (e->type == HB_EVENT_NAVIGATION_COMPLETED || e->type == HB_EVENT_CURSOR_CHANGED) InvalidateRect(hwnd, nullptr, FALSE);
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    LRESULT r = 0;
    if (g_view && hb_win32_handle_message(g_view, hwnd, msg, wp, lp, &r)) return r;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdLine, int show) {
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"HBCoreDemo";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"HB Core", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 900, 650, nullptr, nullptr, inst, nullptr);
    if (!hwnd) return 1;

    HB_CoreConfig cc = {sizeof(HB_CoreConfig), 1, 0, 512, 2, 0, nullptr};
    g_core = hb_core_create(&cc);
    HB_ViewConfig vc = {sizeof(HB_ViewConfig), 800, 600, 1.0f};
    g_view = hb_view_create(g_core, &vc);
    hb_view_set_event_callback(g_view, onEvent, hwnd);
    hb_win32_attach(g_view, hwnd);

    std::string url;
    if (cmdLine && *cmdLine) {
        int n = WideCharToMultiByte(CP_UTF8, 0, cmdLine, -1, nullptr, 0, nullptr, nullptr);
        std::string path(static_cast<size_t>(n > 0 ? n - 1 : 0), '\0');
        WideCharToMultiByte(CP_UTF8, 0, cmdLine, -1, &path[0], n, nullptr, nullptr);
        if (path.size() >= 2 && path.front() == '"' && path.back() == '"') path = path.substr(1, path.size() - 2);
        for (auto& ch : path) if (ch == '\\') ch = '/';
        url = path.find("://") != std::string::npos ? path : "file:///" + path;
    }
    if (url.empty()) hb_view_load_html(g_view, "<title>HB Core</title><h1>Hello HB Core</h1><p>Independent browser engine.</p>");
    else hb_view_load_url(g_view, url.c_str());
    ShowWindow(hwnd, show);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) { TranslateMessage(&m); DispatchMessageW(&m); }
    hb_view_destroy(g_view);
    hb_core_destroy(g_core);
    return 0;
}
