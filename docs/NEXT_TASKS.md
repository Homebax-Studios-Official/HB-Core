# Next tasks (in order)

1. Build on Windows (MSVC x64) - first compile of `platform/windows` and `examples/win32_demo.cpp`; fix portability issues;
   verify `HBCORE_TARGET_WIN7` with a real toolchain and test on Windows 7/8/10/11.
2. Android: NDK build (arm64-v8a), JNI + Kotlin wrapper, `ANativeWindow` presenter; touch input API.
3. Layout 0.4: inline-block, floats, `position`, flexbox, tables, percent heights, `calc()`; text selection and caret.
4. HB.Network 0.3: HTTP/1.1 + TLS (library chosen after license/compatibility review), cookies, HTTP cache,
   downloads (download events), redirects; `http(s)` navigation; remote `<img>`/stylesheets.
5. Fuzzing (libFuzzer) for tokenizer, CSS parser, PNG decoder, TrueType reader; recursion depth limits.
6. JPEG/GIF/WebP decoders, SVG (basic), OpenType/CFF fonts, per-glyph font fallback.
7. HB.JavaScript 0.5 (lexer, parser, AST, bytecode, VM, GC) with Test262 harness; DOM bindings; console events.
8. Web Platform Tests subset runner in `tools/`.
