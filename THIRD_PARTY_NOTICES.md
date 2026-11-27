# Third-party notices

Rule: a third-party copyright, author or license notice is never removed. A third-party
component is never presented as an HB Core implementation. A dependency whose license is
incompatible with the HB Core license or with distribution requirements is not used until
the legal compatibility is resolved.

## Shipped with HB Core 0.1.0

HB Core links **no** third-party runtime library (0.1.0 and 0.2.0). The engine uses only the C++20
standard library. The PNG/inflate decoder and the TrueType reader are own implementations.

**System fonts:** 0.2.0 loads TrueType font files that are already installed on the machine (e.g. Segoe UI/Arial,
Liberation, DejaVu, Noto, Roboto). No font file is bundled or redistributed with HB Core; each font stays under its own license.

### font8x8 (glyph data) - public domain
| Field | Value |
|---|---|
| Name | font8x8 (`font8x8_basic`) |
| Version | n/a (ASCII glyph set) |
| Author | Daniel Hepper (based on the public-domain IBM PC 8x8 font data) |
| License | Public domain |
| Source | https://github.com/dhepper/font8x8 |
| Changes | ASCII table re-typed into `core/graphics/bitmap_font.cpp`; accent marks and code point folding (Czech/German/French letters, typographic punctuation) added by Homebax Studios. Individual glyph bytes should be compared with upstream before a 1.0 release. |
| Obligations | None. Attribution kept here as a courtesy. |

## Test-only (not shipped, not linked into the SDK)

### GoogleTest
| Field | Value |
|---|---|
| Name | GoogleTest |
| Version | 1.15.2 (downloaded at configure time when no system copy is found) |
| Author | Google LLC |
| License | BSD 3-Clause |
| Source | https://github.com/google/googletest |
| Changes | None |
| Obligations | Keep the BSD-3-Clause notice if GoogleTest is redistributed. It is not redistributed with the SDK. |

If GoogleTest cannot be found or downloaded, the tests build against a small built-in
GoogleTest-compatible harness (`tests/mini_gtest.h`, written by Homebax Studios).

## Planned dependencies (not yet used)
FreeType, HarfBuzz, ICU, SQLite, a TLS library (OpenSSL or similar), zlib, libpng,
libjpeg-turbo, libwebp. Each will be added here with name, version, author, license, source,
changes and obligations before it is used.
