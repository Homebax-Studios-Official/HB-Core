# Security

## Status of HB Core 0.1.0
HB Core 0.1.0 is an early foundation. **Do not use it to render untrusted web content in a
product yet.** No sandbox, no process/site isolation, no TLS and no JavaScript exist in this
version, so the attack surface is limited to the HTML/CSS parsers, layout, the rasterizer and
the file loader. Memory-safety hardening has had only basic testing.

## What exists
- No JavaScript engine and no network access: web content cannot reach system functions.
- `loadURL` accepts only `file://`, `data:text/html` and `about:blank`; http(s) is rejected.
- Document size is capped by `CoreConfig::memoryLimitMB / 8`.
- The C ABI never lets exceptions cross the boundary and validates null handles.
- The unit tests, including parser garbage-input and deep-nesting tests, pass under
  AddressSanitizer + UBSan on Linux x64 (GCC 13).

## What does NOT exist yet (and is therefore not claimed)
Same-Origin Policy, CORS, CSP, sandbox, process and site isolation, permission management,
secure cookie/storage, certificate validation, download restrictions, fuzzing, stack-depth
limits for extremely deep DOM trees (see docs/KNOWN_ISSUES.md).

## Reporting a vulnerability
Report privately to Homebax Studios through the contact channels listed on the Homebax
Studios website. Please include version, platform, a minimal test case and the impact.
Do not open a public issue for an unpatched vulnerability.
