# Integrating HB Core into HB Browser

> The HB Browser repository was not available when HB Core 0.1.0 was produced, so the steps below are
> generic. Before applying them, check HB Browser's C++ standard (HB Core needs C++20 or a separate target),
> CMake version (>= 3.20), existing `third-side/` contents and how WebView2 is wired.

## 1. Add the sources
Unzip so that the tree is `third-side/HB-Core/`. Nothing outside this folder is touched.

## 2. CMake
```cmake
set(HBCORE_BUILD_TESTS OFF)
set(HBCORE_BUILD_EXAMPLES OFF)
set(HBCORE_BUILD_BENCHMARKS OFF)
add_subdirectory(third-side/HB-Core)
target_link_libraries(HBBrowser PRIVATE HB::Core)         # shared
```
Make it optional so the WebView2 path stays the default:
```cmake
option(HB_USE_HBCORE "Build the HB Core engine backend" OFF)
if(HB_USE_HBCORE)
    add_subdirectory(third-side/HB-Core)
    target_link_libraries(HBBrowser PRIVATE HB::Core)
    target_compile_definitions(HBBrowser PRIVATE HB_HAS_HBCORE=1)
endif()
```
HB Browser is described as C#-based UI + CMake: from C#, use the C ABI through P/Invoke
(`hb_core_create`, `hb_view_create`, ...). Pixel data from `hb_view_get_pixels` is RGBA8, top-down.

## 3. Engine abstraction (recommended, not part of HB Core)
Put a small interface in HB Browser (e.g. `IWebEngine`: `LoadUrl`, `LoadHtml`, `GoBack`, `GoForward`, `Reload`, `Resize`,
events) with two implementations: the existing WebView2 one (unchanged) and an HB Core one calling the C ABI.
HB Browser can then switch or replace engines without rewriting its UI.

## 4. Presenting pixels
HB Core 0.1.0 has no window presenter. On each frame (or after events) call `hb_view_render`/`hb_view_get_pixels`
and blit into the host surface (Windows: `StretchDIBits`/D3D texture upload; remember RGBA -> BGRA).
Forward `resize`, DPI changes (`hb_view_set_device_pixel_ratio`) and scrolling (`hb_view_scroll_by`).

## 4b. Input and window helpers (0.2.0)
Forward host events to `hb_view_mouse_move/down/up`, `hb_view_mouse_wheel`, `hb_view_key_down`, `hb_view_set_focus`
(coordinates in viewport CSS px = window pixels / DPR). Use `hb_view_get_cursor` after `HB_EVENT_CURSOR_CHANGED`.
Handle `HB_EVENT_NEW_WINDOW_REQUESTED` (open a tab) and `HB_EVENT_CONTEXT_MENU_REQUESTED` (show your menu) yourself.
On Windows `platform/windows/hb_win32.*` (target `HB::Win32`) does presenting and message translation for a plain HWND;
it is **not compiled or tested yet** - expect small fixes on first build. `examples/win32_demo.cpp` shows the wiring.

## 5. What will not work yet
http(s) pages, JavaScript, downloads, permissions, media, touch, text selection, forms. HB Browser must keep WebView2
(or another engine) for real web browsing until those milestones are done.

## 6. License
The HB Browser exception in LICENSE section 5 covers integration and distribution within HB Browser as published
by Homebax Studios.
