Stack-backed list feature suite

Goal:
- Exercise the desired class-template list syntax as a passing feature test.
- Covers `template<typename T> class`, `List<int>` variable declarations, inline constructor/destructor bodies, inline templated member functions, and `numbers.push(...)` method-call sugar.

Usage:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\\run.ps1 -Suite features/List

