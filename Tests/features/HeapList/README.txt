Heap-backed list feature suite

Goal:
- Exercise a heap-backed class-template list as a passing feature test.
- Covers `template<typename T> class`, heap allocation with `malloc`/`realloc`/`free`, inline constructor/destructor bodies, templated member functions, and growth beyond the initial capacity.

Usage:
- powershell -NoProfile -ExecutionPolicy Bypass -File Tests\\run.ps1 -Suite features\\HeapList

