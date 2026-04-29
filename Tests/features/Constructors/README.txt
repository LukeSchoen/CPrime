Constructors feature suite

Goal:
- Resolve constructor targets from `TypeName()` declared inside the struct/class body.
- Support out-of-class definitions as `<TypeName>::<TypeName>()`.
- Apply automatically to local struct/union variables.
- Leave types without a constructor unchanged.

Usage:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\\run.ps1 -Suite features/Constructors

