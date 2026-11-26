# Architecture (0.1.0)

```
HTML text
  -> HB.HTML  tokenizer -> tree builder
  -> HB.DOM   Document / Element / Text
  -> HB.CSS   stylesheets (<style>, local <link>) + UA sheet -> cascade -> ComputedStyle
  -> HB.Layout  box tree (arena-allocated) -> block/inline layout -> line boxes
  -> HB.Paint   display list (document coordinates, CSS px)
  -> HB.Graphics  CPU Surface (RGBA8) + bitmap font -> pixels / PNG
```
`core/engine/page.*` orchestrates this pipeline (an addition to the requested folder list: the
pipeline needs a home that depends on all stages while the public API depends only on it).
`core/engine/hb_core.cpp` implements `HB::Core`/`HB::View`; `bindings/c/hb_core_c.cpp` wraps them in the C ABI.

## Modules and dependencies
Each module is a static library `hb_<name>` with its own test executable.

| Library | Depends on | Responsibility |
|---|---|---|
| hb_base | - | Result/Error, Logger, Signal, TaskScheduler, Arena, strings/UTF-8 |
| hb_dom | base | Node tree |
| hb_html | dom | Tokenizer, tree builder |
| hb_css | dom | Parser, selectors, cascade, ComputedStyle, UA stylesheet |
| hb_graphics | css (Color) | Surface, bitmap font, PNG |
| hb_layout | css, graphics (font metrics) | Box tree, block/inline layout |
| hb_paint | layout, graphics | Display list + rasterization |
| hb_engine | html, paint | Page pipeline |
| hbcore / hbcore_static | engine | Public C++ API + C ABI |

Folders `compositor`, `javascript`, `webapi`, `network`, `media`, `storage`, `security`,
`accessibility`, `devtools` exist as placeholders with a README and **no code**.

## Decisions
- **Layout units** are CSS px (float); rasterization scales by device pixel ratio.
- **Margins** use the classic collapse rules; layout is computed in parent-relative coordinates and made absolute during painting.
- **Fonts**: a built-in bitmap font keeps 0.1.0 dependency-free; FreeType/HarfBuzz/ICU come next.
- **Errors**: `Result<T>`/`Error` instead of exceptions across module boundaries; the C ABI additionally catches everything.
- **Public C++ API** is pimpl-based; use the C ABI where binary stability matters.
- **Ownership**: the DOM owns nodes via `unique_ptr`; layout boxes and styles live in the `LayoutTree` arena.
- **UI separation**: no UI exists in the engine; the host receives pixels (`Framebuffer`) and events.
