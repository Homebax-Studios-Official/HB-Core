# Support matrix - HB Core 0.2.0

Legend: **Yes** = implemented, **Partial** = works for a documented subset, **No** = not implemented.
"Tested" means covered by automated tests in `tests/` on Linux x64 (GCC 13). A successful build is
not evidence of compatibility. Web Platform Tests and Test262 are **not** run yet.

## Engine modules
| Area | Implemented | Tested | Known gaps / bugs |
|---|---|---|---|
| HB.Base (result, log, signal, scheduler, arena, strings/UTF-8) | Yes | Yes | Scheduler is not yet used by the page pipeline |
| HTML tokenizer | Partial | Yes | Hand-written, not the spec state machine; limited named entities (~35); no CDATA/foreign content; bogus-comment handling simplified |
| HTML tree builder | Partial | Yes | No adoption agency algorithm (misnested `<b><i>` not repaired), no table foster parenting, no `<template>`, no SVG/MathML, UTF-8 only, no document.write |
| DOM | Partial | Yes | Tree, attributes, id/class lookup. No events, MutationObserver, Shadow DOM, live collections, DOM mutation API beyond append/remove |
| CSS parser | Partial | Yes | Rules, declarations, `!important`, comments, `@media (min/max-width)`. Other at-rules skipped (`@import`, `@font-face`, `@keyframes`, `@supports`) |
| CSS selectors | Partial | Yes | Type, class, id, attribute (all operators), descendant/child/sibling combinators, `:first-child :last-child :only-child :root :empty`. Other pseudo-classes/elements never match |
| CSS cascade / inheritance | Partial | Yes | Origin, importance, specificity, order, inline style, inheritance, `var()` and custom properties, limited `inherit`. No `initial/unset`, `calc()`, `@layer`, `:is()/:not()` |
| CSS properties | Partial | Yes | display, color, background(-color), font-size/weight/style, text-align/decoration, line-height, white-space, margin/padding, border (+ width/style/color), width/height/min/max-width, overflow(hidden), visibility, opacity (color alpha only), list-style-type. No fonts families, float, position, flex/grid, transforms, gradients, images, border-radius, shadows |
| Layout: block flow, margin collapsing | Yes | Yes | Empty-block self-collapsing not handled |
| Layout: inline text | Partial | Yes | Greedy wrapping, whitespace collapsing, `<br>`, `pre`. No inline-block, inline margins/padding/borders, vertical-align, bidi, hyphenation, `overflow-wrap` options |
| Layout: flexbox, grid, tables, floats, positioning | No | - | `display:flex/grid/table*` are laid out as plain blocks |
| Scrolling | Partial | Yes | Viewport scroll via API, wheel, keys, fragments; no scroll containers, no scrollbars |
| Text rendering | Partial | Yes | Own TrueType (glyf) reader + anti-aliased rasterizer using system fonts (sans/serif/mono, bold/italic, synthesized when missing); unhinted, no kerning/ligatures/shaping/bidi, no OpenType-CFF, no per-glyph fallback. Without a font file: built-in 8x8 bitmap fallback (other scripts show `?`) |
| Paint / display list | Partial | Yes | Backgrounds, borders, text, list markers, clipping. No stacking contexts, z-index, opacity groups, transforms |
| Compositor | No | - | Single-layer CPU render |
| Raster: CPU software | Yes | Yes | RGBA8, DPR scaling. Not SIMD-optimised |
| Graphics: Direct3D 11 / Vulkan / OpenGL | No | - | Planned |
| Image decoding + `<img>` | Partial | Yes | PNG only (own decoder: all color types, depths 1-16, tRNS, Adam7). `<img>` inline/block, width/height, aspect ratio, `data:`/file sources, `[alt]` fallback. No JPEG/GIF/WebP/SVG, no `srcset`/`object-fit` |
| JavaScript (lexer/parser/VM/GC/Promises/modules) | No | - | `executeJavaScript` returns Unsupported |
| Web APIs (Fetch, XHR, timers, Workers, Canvas, Storage, IndexedDB, Service Workers, WebSocket) | No | - | |
| WebGL 1/2, WebGPU, WebAssembly, WebRTC | No | - | |
| Network (DNS/TCP/UDP/HTTP/1.1/2/3/QUIC/TLS/cookies/cache) | No | - | `http(s)` navigation fails with Unsupported |
| Navigation | Partial | Yes | `file://`, `data:text/html` (percent/base64), `about:blank`; link clicks, relative URL resolution, `#fragment` scrolling, history with scroll restore. Local `<link rel=stylesheet>` and images for file pages |
| Storage (cookies, Web Storage, IndexedDB, Cache API, SQLite) | No | - | |
| Security (SOP, CORS, CSP, sandbox, process/site isolation, permissions) | No | - | See SECURITY.md |
| Accessibility tree / screen readers | No | - | |
| DevTools | No | - | |
| Media (audio/video, HW decode) | No | - | |
| Adaptive Resource Manager | Partial | Yes | Only a configurable document-size cap from `memoryLimitMB`; no CPU/RAM/GPU monitoring, throttling or suspension |

## API
| API | Status | Notes |
|---|---|---|
| C++ API (`Core`, `View`, events, `Framebuffer`) | Yes (0.x, unstable) | Navigation/loading/title/URL/error events. **Not implemented:** console, download, permission, new-window events |
| C ABI (`hb_core_c.h`, ABI v1) | Yes | Opaque handles, no exceptions across boundary |
| Input: mouse, wheel, keyboard scrolling, focus, hover/cursor, link click, context-menu and new-window events | Yes | Tested. Hit testing is box/line based |
| Input: touch, text selection, caret, form controls, drag, IME | No | |
| Download and permission events | No | Need networking/JS first |
| Native window embedding | Partial | Win32 GDI presenter + message translation in `platform/windows` (**written, not compiled/tested**). Elsewhere the host blits `Framebuffer` pixels (docs/INTEGRATION.md) |
| Java/Kotlin (JNI) wrapper | No | |

## Platforms
| Platform | Status |
|---|---|
| Linux x86-64, GCC 13, Ninja | Built and tested (Debug, Release, ASan/UBSan, 10 test executables) |
| Linux ARM64 | Not tried (code is portable C++20; no platform-specific code) |
| Windows 7-11 x64 | **Not built or tested.** Presets/`HBCORE_TARGET_WIN7` and the Win32 presenter are provided but unverified (no Windows compiler was available) |
| Android ARM64 | **Not built or tested.** NDK preset provided but unverified; no JNI layer |
