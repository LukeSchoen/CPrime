Constructors feature suite

Goal:
- Resolve constructor targets from `TypeName()` declared inside the struct/class body.
- Support out-of-class definitions as `<TypeName>::<TypeName>()`.
- Apply automatically to local struct/union variables.
- Leave types without a constructor unchanged.

Usage:
- Compatibility\tests\test.exe -Suite features/Constructors

