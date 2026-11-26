# Next tasks (in order)

1. Run the build on Windows (MSVC, x64) and fix portability issues; verify `HBCORE_TARGET_WIN7` with a real toolchain.
2. Run the build with the Android NDK (arm64-v8a); add JNI + Kotlin wrapper in `bindings/android`.
3. Native presenters in `platform/*` (GDI/D3D blit, X11/Wayland helper, ANativeWindow) + input event API
   (mouse/keyboard/touch/focus), hit testing and link navigation.
4. FreeType/HarfBuzz/ICU text pipeline; record dependencies in THIRD_PARTY_NOTICES.md.
5. Image decoding and `<img>`.
6. Import Web Platform Tests subset runner (`tools/`), start tracking pass rates in SUPPORT_MATRIX.md.
7. Recursion depth limits and a fuzzing harness for tokenizer/parser/CSS (libFuzzer).
8. Begin HB.JavaScript (lexer, parser, AST) with Test262 harness.
