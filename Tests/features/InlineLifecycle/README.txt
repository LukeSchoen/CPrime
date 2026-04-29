Inline lifecycle feature suite

Goal:
- Support inline `TypeName()` and `~TypeName()` bodies inside struct/class definitions.
- Constructors run at local declaration time.
- Destructors run on scope unwind.

Usage:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\\run.ps1 -Suite features/InlineLifecycle

