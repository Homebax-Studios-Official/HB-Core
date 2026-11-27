# Development status

Updated: 2026-11-26 - version 0.2.0

## Verified in this environment
- Linux x86-64, GCC 13.3, CMake 4.4, Ninja: Debug (with `-Werror`) and Release builds.
- 10 test executables (base, dom, html, css, graphics, image, url, layout, engine, api incl. input) - all pass, also under ASan + UBSan, and on the built-in fallback harness.
- GoogleTest 1.15.2 (downloaded at configure time).
- `hb_demo_render` renders the required "Hello HB Core" page to a PNG (real pixels, not source text); `hb_demo_c` uses only the C ABI.
- `cmake --install` produces headers, `libhbcore.so`, and a CMake package.

## Done in 0.2.0 (on top of 0.1.0)
TrueType text (own reader/rasterizer + system font discovery), PNG decoder + `<img>`, input events, hit testing, link
navigation, fragment scrolling, URL resolution, C ABI additions, Win32 presenter/demo (written only, unverified).

## Done in 0.1.0
CMake project, C++20 foundation, logging, error handling, arena memory, task scheduler, event system,
HTML tokenizer + tree builder, DOM, CSS parser/cascade, layout, display list, CPU renderer, PNG output,
C++ API, C ABI, unit tests, benchmark, demos, documentation.

## Not verified / not done
Windows and Android builds (incl. the new Win32 presenter), ARM64, Web Platform Tests, Test262, everything listed as "No" in SUPPORT_MATRIX.md.
The HB Browser repository was **not available** while this was written, so its architecture,
CMake setup and WebView2 integration were not analysed (see docs/INTEGRATION.md).

## Benchmark (single run, Release, this sandbox - not a promise)
`benchmarks/hb_bench_pipeline`, generated ~349 KiB document (2000 headings/paragraphs/lists):
bitmap font: parse 8.8 ms, style + layout 22.5 ms, paint + raster (1024x768) 3.8 ms.
With Liberation TrueType fonts: style + layout 24.9 ms, paint 2.7 ms with a cold glyph cache, 1.0 ms warm.
Machine- and build-specific; no RAM figures were measured.

## Also verified
- Offline fallback: with `-DHBCORE_FETCH_GTEST=OFF` the same 8 test executables pass on the built-in harness.
- `cmake --install` + `find_package(HBCore)` + `target_link_libraries(... HB::Core)` works from an external project.
