# Instructions for AI coding agents and contributors

HB Core is an independent browser engine by Homebax Studios. Read `docs/DEVELOPMENT_STATUS.md`,
`docs/NEXT_TASKS.md` and `docs/ARCHITECTURE.md` first; continue from there.

Rules
1. Work only inside `third-side/HB-Core`. Never modify or remove HB Browser's own engine or its WebView2 integration.
2. No Chromium/Blink/WebKit/Gecko/WebView2/CEF/Electron/Qt WebEngine code. No pseudocode in sources; no empty
   classes pretending to be features; no fake test results.
3. Every feature needs a test. After each change: build, run `ctest`, fix failures, then update
   `SUPPORT_MATRIX.md`, `docs/DEVELOPMENT_STATUS.md`, `docs/KNOWN_ISSUES.md`, `docs/NEXT_TASKS.md`, `CHANGELOG.md`.
4. Never mark a feature "done" in docs unless it is implemented and tested. Do not claim unmeasured performance.
5. New third-party dependencies: record them in `THIRD_PARTY_NOTICES.md` (name, version, author, license, source,
   changes, obligations) and check license compatibility first. Never remove foreign copyright notices.
6. Keep the public C ABI (`hb_core_c.h`) stable; bump `HB_ABI_VERSION` on any breaking change.
7. Security-relevant decisions (sandboxing, TLS, process model) must not be improvised - document and ask.
8. Keep modules small and independently testable (`hb_<module>` static libraries, one test executable each).
9. Never delete working code only to rewrite it.

Build: `cmake --preset linux-debug && cmake --build --preset linux-debug && ctest --preset linux-debug`
Sanitizers: configure with `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` plus matching linker flags.
