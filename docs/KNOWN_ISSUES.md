# Known issues (0.1.0)

1. Very deep DOM trees (hundreds of thousands of nested elements) can overflow the call stack in recursive code
   (destruction, `textContent`, layout, painting). 3000 levels are tested; a depth limit is not implemented yet.
2. The tokenizer/tree builder are not the full HTML5 algorithms (see SUPPORT_MATRIX.md); misnested formatting
   elements and tables are not repaired like in browsers.
3. Text uses an 8x8 bitmap font: other scripts render `?`; metrics are a fixed 0.6 em advance; glyph data was
   re-typed and should be verified against upstream font8x8.
4. Body background is both propagated to the canvas and painted on the body box (harmless without alpha).
5. Inline `margin/padding/border` are ignored; `display:flex/grid/table` are laid out as blocks.
6. `loadURL` for `file://` reads the whole file synchronously on the calling thread.
7. Windows/Android/ARM64 builds and the Windows 7 toolchain are unverified.
8. Percent heights, `position`, `float`, `calc()` and `@import` are not supported.
9. The CPU renderer is not optimised (no SIMD, full-document display list). Benchmarks exist only as a tool.
