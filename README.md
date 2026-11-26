# HB Core

**Independent web browser engine SDK by Homebax Studios** - version 0.1.0
Author: Ondrej Jurasik - Copyright (c) 2026 Homebax Studios - [Non-Commercial License](LICENSE)

HB Core is written from scratch in C++20 (HTML parser, DOM, CSS, layout, painting, CPU rasterizer).
It is an embeddable SDK for **HB Browser** - it has no tab bar, address bar, bookmarks or
history UI. It is **not** a wrapper around Chromium, WebKit, Gecko, WebView2, CEF, Qt WebEngine or Electron.

> **Honest status:** 0.1.0 is the *foundation milestone*. It renders static HTML + CSS pages from
> `file://`, `data:` URLs or strings into an RGBA framebuffer. It has **no JavaScript, no networking,
> no GPU backend and no sandbox yet.** See [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) and
> [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md).

## Quick start (Linux)
```sh
cmake --preset linux-release        # or: cmake -G Ninja -S . -B build
cmake --build --preset linux-release
ctest --preset linux-release
./build/linux-release/examples/hb_demo_render                # renders the "Hello HB Core" page -> hb_core_demo.png
./build/linux-release/examples/hb_demo_render examples/pages/showcase.html showcase.png 560 520
```
Requirements: CMake >= 3.20, Ninja (or another generator), a C++20 compiler. GoogleTest is used
when installed or downloadable; otherwise a tiny built-in harness runs the same tests.

## C++ API
```cpp
#include <hbcore/hb_core.h>

HB::Core core;
HB::CoreConfig config;
config.memoryLimitMB = 512;
core.initialize(config);

HB::View view(core);
view.setEventHandler([](const HB::ViewEvent& e) { /* navigation, title, url, error events */ });
view.loadHTML("<h1>Hello HB Core</h1>");
HB::Framebuffer fb = view.render();   // RGBA8 pixels for your window
```
`executeJavaScript()` exists but returns `ErrorCode::Unsupported` until HB.JavaScript is written.

## C ABI
`hbcore/hb_core_c.h` - opaque handles (`HB_Core*`, `HB_View*`), `hb_core_create`, `hb_view_create`,
`hb_view_load_url`, `hb_view_load_html`, `hb_view_resize`, `hb_view_get_pixels`, `hb_view_destroy`,
`hb_core_destroy`, ... See `examples/demo_c.c`.

## CMake integration
```cmake
add_subdirectory(third-side/HB-Core)
target_link_libraries(HBBrowser PRIVATE HB::Core)       # shared: hbcore.dll / libhbcore.so
# or: HB::CoreStatic
```
Details and the WebView2 coexistence plan: [docs/INTEGRATION.md](docs/INTEGRATION.md).

## Layout
`include/hbcore` public headers - `core/<module>` engine modules - `bindings/c` C ABI -
`tests` - `benchmarks` - `examples` - `docs` - `platform` (placeholders) - `resources/branding`.
Architecture: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). Status: [docs/DEVELOPMENT_STATUS.md](docs/DEVELOPMENT_STATUS.md).
