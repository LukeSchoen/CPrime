Template feature suite

Scope:
- C++ class and function templates, deduction, specialization, and member templates.
- Self-contained executable regressions and explicit external CommonLib compile probes.

Run:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\run.ps1 -Suite features/Templates -BuildManifestPath path\to\cl\builds\manifest\Release-x64.json
- See Tests/README.md for metadata and manifest setup.

