# Known issues (0.1.0)

1. Very deep DOM trees (hundreds of thousands of nested elements) can overflow the call stack in recursive code
   (destruction, `textContent`, layout, painting). 3000 levels are tested; a depth limit is not implemented yet.
2. The tokenizer/tree builder are not the full HTML5 algorithms (see SUPPORT_MATRIX.md); misnested formatting
   elements and tables are not repaired like in browsers.
3. Text: outline fonts are loaded from system font files (no font bundled). No hinting, kerning, ligatures, shaping,
   bidi, complex scripts or font fallback per glyph (missing glyphs show the font's .notdef box). OpenType/CFF (`.otf`)
   fonts are not supported - only TrueType outlines. Without any font file the 8x8 bitmap fallback is used (other scripts show `?`;
   its glyph data was re-typed and should be verified against upstream font8x8).
4. Body background is both propagated to the canvas and painted on the body box (harmless without alpha).
5. Inline `margin/padding/border` are ignored; `display:flex/grid/table` are laid out as blocks.
6. `loadURL` for `file://` reads the whole file synchronously on the calling thread.
7. Windows/Android/ARM64 builds and the Windows 7 toolchain are unverified.
8. Percent heights, `position`, `float`, `calc()` and `@import` are not supported.
9. The CPU renderer is not optimised (no SIMD, full-document display list). Benchmarks exist only as a tool.
10. Images: only PNG is decoded (no JPEG/GIF/WebP/SVG); chunk CRCs are not verified; `<img>` sizes in percent, `srcset`,
    `object-fit` and `loading` are ignored. Images are decoded synchronously during layout.
11. Fragment navigation needs a target that has a box or text (empty `<a name=x></a>` cannot be located).
12. Links whose base is `about:blank`/`data:` cannot resolve relative URLs (nothing happens on click).
13. `:hover`, `:active`, `:focus` selectors never match (no state tracking yet); text selection, caret and form controls do not exist.
14. The Win32 helpers/demo were written without a Windows compiler; the first Windows build may need fixes.
15. Fonts are process-wide (shared by all `Core` instances).
