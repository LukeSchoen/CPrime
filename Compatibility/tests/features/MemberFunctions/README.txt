Method-call sugar feature suite

Goal:
- `foo.bar(a, b)` rewrites to `bar(&foo, a, b)`.
- `ptr->bar(a, b)` remains out of scope.

Usage:
- Compatibility\tests\test.exe -Suite features/MemberFunctions

