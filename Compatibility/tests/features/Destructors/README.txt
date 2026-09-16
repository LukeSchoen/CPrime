Destructors feature suite

Goal:
- Resolve cleanup targets from `~TypeName()` declared inside the struct/class body.
- Support out-of-class definitions as `<TypeName>::~<TypeName>()`.
- Apply automatically to local struct/union variables.
- Leave types without a destructor unchanged.

Usage:
- Compatibility\tests\test.exe -Suite features/Destructors

