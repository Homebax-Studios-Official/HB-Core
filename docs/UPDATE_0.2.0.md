# Aktualizace 0.1.0 -> 0.2.0

Rozbal zip do kořene repozitáře (vznikne `third-side/HB-Core/...`) a přepiš existující soubory. Žádný soubor se nemaže.

## Nové soubory (17)
- `HB-Core/HB-Core/core/css/font_family.hpp`
- `HB-Core/HB-Core/core/engine/url.cpp`
- `HB-Core/HB-Core/core/engine/url.hpp`
- `HB-Core/HB-Core/core/graphics/fonts.cpp`
- `HB-Core/HB-Core/core/graphics/fonts.hpp`
- `HB-Core/HB-Core/core/graphics/image.cpp`
- `HB-Core/HB-Core/core/graphics/image.hpp`
- `HB-Core/HB-Core/core/graphics/truetype.cpp`
- `HB-Core/HB-Core/core/graphics/truetype.hpp`
- `HB-Core/HB-Core/examples/win32_demo.cpp`
- `HB-Core/HB-Core/platform/windows/hb_win32.cpp`
- `HB-Core/HB-Core/platform/windows/hb_win32.h`
- `HB-Core/HB-Core/tests/test_image.cpp`
- `HB-Core/HB-Core/tests/test_input.cpp`
- `HB-Core/HB-Core/tests/test_png_data.h`
- `HB-Core/HB-Core/tests/test_url.cpp`
- `HB-Core/HB-Core/tools/gen_test_png.py`

## Změněné soubory – přepsat (37)
- `HB-Core/HB-Core/CHANGELOG.md`
- `HB-Core/HB-Core/CMakeLists.txt`
- `HB-Core/HB-Core/README.md`
- `HB-Core/HB-Core/SECURITY.md`
- `HB-Core/HB-Core/SUPPORT_MATRIX.md`
- `HB-Core/HB-Core/THIRD_PARTY_NOTICES.md`
- `HB-Core/HB-Core/benchmarks/bench_pipeline.cpp`
- `HB-Core/HB-Core/bindings/c/hb_core_c.cpp`
- `HB-Core/HB-Core/core/base/strings.cpp`
- `HB-Core/HB-Core/core/base/strings.hpp`
- `HB-Core/HB-Core/core/css/style.cpp`
- `HB-Core/HB-Core/core/css/style.hpp`
- `HB-Core/HB-Core/core/css/ua_stylesheet.cpp`
- `HB-Core/HB-Core/core/engine/hb_core.cpp`
- `HB-Core/HB-Core/core/engine/page.cpp`
- `HB-Core/HB-Core/core/engine/page.hpp`
- `HB-Core/HB-Core/core/graphics/surface.cpp`
- `HB-Core/HB-Core/core/graphics/surface.hpp`
- `HB-Core/HB-Core/core/layout/layout.cpp`
- `HB-Core/HB-Core/core/layout/layout.hpp`
- `HB-Core/HB-Core/core/paint/display_list.hpp`
- `HB-Core/HB-Core/core/paint/painter.cpp`
- `HB-Core/HB-Core/docs/ARCHITECTURE.md`
- `HB-Core/HB-Core/docs/DEVELOPMENT_STATUS.md`
- `HB-Core/HB-Core/docs/IMPLEMENTATION_PLAN.md`
- `HB-Core/HB-Core/docs/INTEGRATION.md`
- `HB-Core/HB-Core/docs/KNOWN_ISSUES.md`
- `HB-Core/HB-Core/docs/NEXT_TASKS.md`
- `HB-Core/HB-Core/examples/CMakeLists.txt`
- `HB-Core/HB-Core/examples/demo_render.cpp`
- `HB-Core/HB-Core/examples/pages/showcase.html`
- `HB-Core/HB-Core/include/hbcore/hb_core.h`
- `HB-Core/HB-Core/include/hbcore/hb_core_c.h`
- `HB-Core/HB-Core/include/hbcore/hb_version.h`
- `HB-Core/HB-Core/tests/CMakeLists.txt`
- `HB-Core/HB-Core/tests/test_api.cpp`
- `HB-Core/HB-Core/tools/verify_tree.py`

Po přepsání: `cmake --preset linux-release && cmake --build --preset linux-release && ctest --preset linux-release` (Windows: viz README, preset `windows-msvc-release`).
