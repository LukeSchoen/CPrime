Inline lifecycle feature suite

Goal:
- Support inline `TypeName()` and `~TypeName()` bodies inside struct/class definitions.
- Constructors run at local declaration time.
- Destructors run on scope unwind.

Usage:
- Compatibility\tests\test.exe -Suite features/InlineLifecycle

