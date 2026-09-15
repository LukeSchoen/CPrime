# C++17 remaining scope

Working queue only. `cpp17-coverage.json` maps upstream cases to local tests;
this page lists what still has to be built.

## constexpr object model

| Gap | Reproducer shape |
| --- | --- |
| user-provided constructor is not constant-evaluated | `static_assert(S(1).a == 1)` with `constexpr S(int) : a(v) {}` |
| union member read | `union U{int i;}; constexpr int f(){U u{5}; return u.i;}` |
| bit-field read | `struct B{unsigned a:3;}; constexpr int f(){B b{5}; return (int)b.a;}` |
| reference member | `struct R{int &r;}; constexpr int f(){int x=3; R v{x}; return v.r;}` |
| lifetime escape accepted | `constexpr const int *f(){int x=1; return &x;}` |
| uninitialized member read accepted | `constexpr Bad() : b(2), a(b+1) {}` |

Already passing and retained: aggregate returns and copies, indirect calls,
member allocation and provenance, subobject mutation, class ranges and
sentinels, selection and loop scopes, discarded runtime calls, and the
member-pointer forms.

## Statements and local state

Selection and loop scopes, initialization order across local declarations,
reference binding, pointer bounds, and discarded runtime calls.

## Lexing and literals

- user-defined literals are parsed but not constant-evaluated
- `sizeof(u"ab")` disagrees with the encoded-literal element type
- wide and narrow literals concatenate instead of being rejected

Digit separators, raw strings, Unicode escapes, and the escape diagnostics
already pass.

## Core C++17

- pack expansion nested in a function parameter type (`void f(box<Ts...>)`)
- class template argument deduction from parentheses
- trailing return types whose `decltype` names a dependent call
- `std::is_copy_assignable`

Deleted functions, special-member availability, `is_constructible`,
`is_copy_constructible`, fold expressions, `if constexpr`, lambdas, inline
variables, noexcept function types, and attributes already pass.

## Libraries

- `std::variant`: four alternatives, no copy constructor or copy assignment
- `std::tuple`: variadic, but `make_tuple`/`tie` spell parameter lists to six
  and `get`/`apply` deduce the whole tuple because parameter-position packs are
  unsupported

`std::any` (dynamic type, cast rejection, `emplace`, `swap`, `make_any`),
`std::scoped_lock` (any number of mutexes), `std::shared_mutex` (reader-writer),
and the tuple arity work are covered by retained cases.

## Where the cases live

- `features/Abi/pass` — Microsoft x64 layout, nullptr and record-return facts
  that used to need a second compiler
- `features/Templates/pass` — variadic class, pack and constexpr semantics
- `features/Includes/pass` — runtime header and container behavior
- `features/StdConcurrency/pass` — mutex, lock and thread behavior
