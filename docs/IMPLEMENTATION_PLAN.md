# Implementation plan

| Milestone | Content | Status |
|---|---|---|
| 0.1.0 | Foundation: build, base, HTML, DOM, CSS, layout, CPU render, C/C++ API, tests | Done |
| 0.2.0 | Text: FreeType + HarfBuzz (+ ICU), images (zlib/libpng/libjpeg-turbo), `<img>`, fonts families | Planned |
| 0.3.0 | Input events, hit testing, links, scroll containers, native-window presenters (Win32, X11/Wayland helpers, ANativeWindow) | Planned |
| 0.4.0 | Layout: inline-block, floats, positioning, flexbox; stacking contexts | Planned |
| 0.5.0 | HB.JavaScript: lexer, parser, AST, bytecode, VM, GC, then DOM bindings and event loop; Test262 harness | Planned |
| 0.6.0 | HB.Network: HTTP/1.1 + TLS (library chosen after license review), cookies, cache; Fetch/XHR | Planned |
| 0.7.0 | Security model: origins, SOP/CORS/CSP, process isolation + sandbox design (needs explicit decisions) | Planned |
| 0.8.0 | GPU backends behind the graphics abstraction (D3D11, Vulkan, OpenGL), compositor | Planned |
| 0.9.0 | Storage (SQLite), Web APIs, accessibility tree, DevTools basics, Android JNI + Windows platform layers | Planned |
| 1.0.0 | WPT subset passing, documented compatibility, stable ABI | Planned |
Later: HTTP/2, HTTP/3/QUIC, WebGL, WebGPU, WebAssembly, WebRTC, PWA/Service Workers.
