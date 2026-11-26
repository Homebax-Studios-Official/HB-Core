# Changelog

All notable changes to HB Core. Format: Keep a Changelog. Versioning: 0.x = unstable API.

## [0.1.0] - 2026-11-26
First foundation milestone. See SUPPORT_MATRIX.md for exact status.

### Added
- CMake/Ninja project, CMake Presets, install rules, `HB::Core` / `HB::CoreStatic` targets.
- HB.Base: result/error types, logger, signal (event system), priority task scheduler, arena allocator, string/UTF-8 helpers.
- HB.HTML: tokenizer and tree builder (implied structure, void elements, implied end tags).
- HB.DOM: Document, Element, Text, Comment, DocumentType.
- HB.CSS: tokenizing parser, selectors, cascade, inheritance, `var()`, `@media` (width), colors, lengths, user-agent stylesheet.
- HB.Layout: block flow, margin collapsing, inline text flow, wrapping, lists, `overflow:hidden`.
- HB.Paint / HB.Graphics: display list, CPU software rasterizer, built-in bitmap font, PNG encoder.
- Public C++ API (`hbcore/hb_core.h`) and stable C ABI (`hbcore/hb_core_c.h`, ABI version 1).
- Navigation for `file://`, `data:text/html`, `about:blank`; history; events.
- Tests (GoogleTest), benchmark, demos, documentation.

### Not included (see SUPPORT_MATRIX.md)
JavaScript, networking/TLS, GPU backends, WebGL/WebGPU, media, storage, security sandboxing,
accessibility, DevTools, input events, Android/Windows platform layers.
