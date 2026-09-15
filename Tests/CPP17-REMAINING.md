# C++17 remaining scope

Working queue only: open gaps and the rules for closing them. Completed work is
code, retained cases are the record of what passes, and git is the record of
what changed. Root `cpc.exe` is the only compiler, one process at a time.

## Test loop

```
Tests\test.exe -All -Tier fast        the loop: 25 representative cases, ~1s
Tests\test.exe -Suite features/X      one suite, every retained case
Tests\test.exe -All -Tier pedantic    the full internal inventory, ~20s
Tests\test.exe -Regression            publication gate
scripts\build.exe                     self-host, validate, publish cpc.exe
```

`Tests/tiers.json` lists the fast subset; every other retained internal case
runs in the pedantic tier, so a case leaves the loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## constexpr object model

| Gap | Reproducer shape |
| --- | --- |
| user-provided constructor is not constant-evaluated | `static_assert(S(1).a == 1)` with `constexpr S(int) : a(v) {}` is rejected with "constant expression expected"; member-function calls on the same object work |
| union member read and write-then-read | `union U{int i;}; constexpr int f(){U u{5}; return u.i;}` |
| bit-field read | `struct B{unsigned a:3;}; constexpr int f(){B b{5}; return (int)b.a;}` |
| reference member | `struct R{int &r;}; constexpr int f(){int x=3; R v{x}; return v.r;}` |
| uninitialized member read | `constexpr Bad() : b(2), a(b+1) {}`, masked behind the constructor gap until that is repaired |

## Lexing and literals

- wide and narrow literals concatenate (`"a" L"b"`) instead of being rejected

## Core C++17

- class template argument deduction works only from braced initializers;
  `W w(3);` is rejected
- `std::is_copy_assignable` is missing

## Pack expansion

- a pack expansion inside a template-id in a function's return type
  (`Tup<typename decay<Ts>::type...>`) is rejected
- a non-type pack element used as an explicit template argument in a call
  (`at<I>()...`, `std::get<I>(t)...`) is rejected

These two are all that keep `make_tuple`, `tie` and `apply` spelled out to six
parameters. Deduction from a parameter type (`void f(box<Ts...>)`), a pack
deduced from two positions, and plain `f(v...)` expansion work.

## Libraries

- `std::variant` holds at most four alternatives and has no converting
  constructor (`std::variant<int, double, char> v{2.5}` fails), no
  `get<Index>`, no `get_if` and no `emplace`; assigning a non-copyable
  alternative fails
- `std::tuple` is variadic but `make_tuple`/`tie`/`apply` stop at six
  parameters, and `std::get<double>(tuple)` (get by type) is unsupported
- `std::optional` is unusable in constant expressions, and
  `std::is_copy_constructible<std::optional<NC> >` is wrongly true

## Fold diagnostics

- `(t + 1 + ...)` is accepted although fold operands must be cast-expressions

## Fixed, waiting for a retained case

Reproduced as fixed, still unprotected by the gate:

- user-defined literals constant-evaluate (`2.5_km == 2500.0L`)
- encoded-literal sizes match the Microsoft x64 ABI: `sizeof(u"ab")` 6,
  `sizeof(U"ab")` 12, `sizeof(L"ab")` 6, `sizeof(u'a')` and `sizeof(L'a')` 2,
  `sizeof(U'a')` 4
- a `constexpr` function returning the address of a function-local object is
  rejected
- selection and loop scopes, subobject mutation, initialization order,
  references, pointer bounds and discarded runtime calls in constant
  expressions
- folds over non-type packs, constexpr array element reads inside a fold, a
  thirty-element fold, and rejection of the empty unary `*` fold
- a dependent trailing return type whose `decltype` names a dependent call

## Where the cases live

- `features/Abi/pass` - Microsoft x64 layout, nullptr and record-return facts
  that used to need a second compiler
- `features/Cpp17Gaps/pass` - minimal standalone reproducers for the gaps the
  CL repository probe found; every case is a valid program that must compile,
  link and exit 0
- `features/Declarations/pass`, `features/Statements/pass`,
  `features/Templates/pass`, `features/Includes/pass` - structured bindings,
  selection initializers, variadic class and pack semantics, runtime header and
  container behavior
- `features/StdConcurrency/pass` - mutex, lock and thread behavior

## Rules

- Reproduce first, then repair the shared mechanism, then retain one case. A
  failed case stays in `pass/`, never relabelled as an expected failure.
- Keep tests minimal, deterministic, and fast; reuse an existing case when it
  already proves the behavior.
- Put one-off reproducers in `build/` and delete them once the durable case
  exists.
- Run the exact selected case, then the affected suite, then the fast tier at a
  boundary, and the pedantic tier before publication.
