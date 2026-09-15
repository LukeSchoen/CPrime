# C++17 language support: remaining work

The root `cpc.exe` is the only compiler used for this work, one process at a
time. Tests live in `Tests/`; generated evidence lives in `build/`.

## Test loop

```
Tests\test.exe -All -Tier fast        the loop: 25 representative cases, ~1s
Tests\test.exe -Suite features/X      one suite, every retained case
Tests\test.exe -All -Tier pedantic    the full internal inventory, ~20s
Tests\test.exe -Regression            publication gate (~58 cases)
scripts\build.exe                     self-host, validate, publish cpc.exe
```

`Tests/tiers.json` lists the `fast` subset. Every other retained internal case
runs in the pedantic tier, so a case leaves the loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## Remaining work

### 1. constexpr object evaluation

- A user-provided `constexpr` constructor is never constant-evaluated:
  `static_assert(S(1).a == 1)` and `constexpr S g(1);` with
  `constexpr S(int) : a(v) {}` are rejected with "constant expression
  expected". Member-function calls on the same object work.
- A union or bit-field member of a constexpr object cannot be read; writes to a
  local constexpr object are accepted but never recorded.
- A reference member cannot be initialized in a constexpr aggregate
  ("lvalue expected").
- Returning the address of a function-local object, or of a temporary, from a
  `constexpr` function is accepted instead of rejected.
- Reading a member that declaration order has not yet initialized is accepted.

### 2. constexpr statements and local state

Selection and loop scopes, mutation through subobjects, initialization order,
references, pointer bounds, and discarded runtime calls.

### 3. Literals and diagnostics

- User-defined literals parse but are not constant-evaluated
  (`2.5_km == 2500.0L` fails a `static_assert`).
- `sizeof(u"ab")` does not match the encoded-literal element type.
- Concatenating a wide and a narrow literal (`L"a" "b"`) is accepted.
- Digit separators, raw strings, Unicode escapes, and the Unicode-escape
  diagnostics already pass.

### 4. Core C++17 semantics

- A pack expansion nested in a function parameter type
  (`void f(box<Ts...>)`) is rejected, which is why `std::get`/`std::apply`
  deduce the whole tuple type instead.
- Class template argument deduction works only from braced initializers.
- A trailing return type whose `decltype` names a dependent call does not
  participate in deduction.
- `std::is_copy_assignable` is missing.

### 5. Libraries

- `std::variant` holds four alternatives with no copy constructor or copy
  assignment.
- `std::tuple` is variadic but `make_tuple`/`tie` spell their parameter lists
  up to six, because a pack expansion inside a function's template-id is not
  substituted.

## Rules

- Reproduce first, then repair the shared mechanism, then retain one case. A
  failed case stays in `pass/`, never relabelled as an expected failure.
- Keep tests minimal, deterministic, and fast; reuse an existing case when it
  already proves the behavior.
- Put one-off reproducers in `build/` and delete them once the durable case
  exists.
- Run the exact selected case, then the affected suite, then the fast tier at
  a boundary, and the pedantic tier before publication.
