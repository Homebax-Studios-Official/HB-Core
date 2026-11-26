# Development status

Updated: 2026-11-26 - version 0.1.0 (first milestone)

## Verified in this environment
- Linux x86-64, GCC 13.3, CMake 4.4, Ninja: Debug (with `-Werror`) and Release builds.
- 8 test executables (base, dom, html, css, graphics, layout, engine, api) - all pass, also under ASan + UBSan.
- GoogleTest 1.15.2 (downloaded at configure time).
- `hb_demo_render` renders the required "Hello HB Core" page to a PNG (real pixels, not source text); `hb_demo_c` uses only the C ABI.
- `cmake --install` produces headers, `libhbcore.so`, and a CMake package.

## Done in 0.1.0
CMake project, C++20 foundation, logging, error handling, arena memory, task scheduler, event system,
HTML tokenizer + tree builder, DOM, CSS parser/cascade, layout, display list, CPU renderer, PNG output,
C++ API, C ABI, unit tests, benchmark, demos, documentation.

## Not verified / not done
Windows and Android builds, ARM64, Web Platform Tests, Test262, everything listed as "No" in SUPPORT_MATRIX.md.
The HB Browser repository was **not available** while this was written, so its architecture,
CMake setup and WebView2 integration were not analysed (see docs/INTEGRATION.md).

## Benchmark (single run, Release build, this sandbox - not a promise)
`benchmarks/hb_bench_pipeline`, generated document of ~349 KiB (2000 headings/paragraphs/lists):
parse 14.8 ms, style + layout 42.9 ms, paint + raster of a 1024x768 viewport 6.1 ms.
Numbers depend on machine and build type; compare only on the same machine. No RAM figures were measured.

## Also verified
- Offline fallback: with `-DHBCORE_FETCH_GTEST=OFF` the same 8 test executables pass on the built-in harness.
- `cmake --install` + `find_package(HBCore)` + `target_link_libraries(... HB::Core)` works from an external project.
