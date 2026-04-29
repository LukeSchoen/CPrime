Template feature suite

Scope:
- Only `template<typename T>` syntax is valid.
- Single type parameter only.
- Function templates only.
- Explicit instantiation required at call site.

Call shape under test:
- `name(int)(args...)`

Run:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\\run.ps1 -Suite features/Templates

