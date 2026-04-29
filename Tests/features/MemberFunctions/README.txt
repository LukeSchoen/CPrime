Method-call sugar feature suite

Goal:
- `foo.bar(a, b)` rewrites to `bar(&foo, a, b)`.
- `ptr->bar(a, b)` remains out of scope.

Usage:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\run.ps1 -Suite features/MemberFunctions

