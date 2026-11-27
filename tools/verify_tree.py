#!/usr/bin/env python3
"""Checks that the HB Core tree has the required files and that every source file carries the copyright header.
Usage: python3 tools/verify_tree.py   (run from anywhere)"""
import pathlib, sys

root = pathlib.Path(__file__).resolve().parent.parent
required = [
    "CMakeLists.txt", "CMakePresets.json", "LICENSE", "README.md", "CHANGELOG.md", "COPYRIGHT.md",
    "THIRD_PARTY_NOTICES.md", "SECURITY.md", "SUPPORT_MATRIX.md", "AGENTS.md",
    "docs/DEVELOPMENT_STATUS.md", "docs/IMPLEMENTATION_PLAN.md", "docs/ARCHITECTURE.md",
    "docs/KNOWN_ISSUES.md", "docs/NEXT_TASKS.md", "docs/INTEGRATION.md",
    "include/hbcore/hb_core.h", "include/hbcore/hb_core_c.h", "platform/windows/hb_win32.h",
    "resources/branding/hbcore-logo.svg", "resources/icons/hbcore-icon.svg",
]
missing = [f for f in required if not (root / f).exists()]
bad_header = []
for ext in ("*.cpp", "*.hpp", "*.h", "*.c"):
    for p in list((root / "core").rglob(ext)) + list((root / "include").rglob(ext)) + list((root / "bindings").rglob(ext)):
        head = p.read_text(encoding="utf-8", errors="replace")[:400]
        if "Homebax Studios" not in head:
            bad_header.append(str(p.relative_to(root)))
for m in missing:
    print("MISSING:", m)
for b in bad_header:
    print("NO COPYRIGHT HEADER:", b)
print("OK" if not missing and not bad_header else "FAILED")
sys.exit(1 if missing or bad_header else 0)
