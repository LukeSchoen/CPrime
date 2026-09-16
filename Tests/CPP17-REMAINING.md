# C++17 remaining scope

Working queue only: open gaps and the rules for closing them. Completed work is
code, retained cases are the record of what passes, and git is the record of
what changed. Root `cpc.exe` is the only compiler, one process at a time.

Every entry below was re-checked against root `cpc.exe` on 2026-09-16 and
records what the compiler does today; the quoted errors are verbatim. Re-verify
before trusting an entry that has aged.

## Test loop

```
Tests\test.exe -All -Tier fast        the loop: the open gaps below, ~0.2s
Tests\test.exe -Suite features/X      one suite, every retained case
Tests\test.exe -Regression            publication gate
scripts\build.exe                     self-host, validate, publish cpc.exe
```

Do not run the pedantic tier. It is close to banned: run it only as the last
and only step of an important confirmation, and avoid it if at all possible.
The affected suite plus `-Regression` is the broad check.

`Tests/tiers.json` lists the fast subset, which is the open work list; every
other retained internal case belongs to the pedantic tier, so a case leaves the
loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## The gate is red on purpose

`features/Cpp17Gaps` carries one case per open gap, so the fast tier is red by
exactly the number of open gaps: 17 cases, 16 in `pass/` and one in `fail/`.
That count is the work list, and it falls as gaps close.

The open-gap cases are the whole of `Tests/tiers.json`'s fast list, so the
routine loop reports the work list instead of hiding it until the pedantic run,
and a green fast run means the queue is empty. Fast is ~0.2s; pedantic carries
every representative case. A fixed case leaves `fast` by not being listed, and
returns only if the routine loop needs that behavior covered on every pass.

Each case names its facility, so a fix starts by running it:

```
Tests\test.exe -Suite features/Cpp17Gaps -Select test_variant_get_if.cpp
Tests\test.exe -Suite features/Cpp17Gaps
```

## constexpr object model

One evaluator, four symptoms. The union, bit-field and reference rows are
independently broken, not merely masked by the constructor gap: each still
fails with the user-provided constructor removed. The union and bit-field
reproducers also cover writes to a local constexpr object, which is why that
bullet no longer stands on its own.

| Gap | Reproducer shape | Observed |
| --- | --- | --- |
| user-provided constructor is not constant-evaluated | `constexpr S(int v) : a(v) {}` then `static_assert(S(1).a == 1)` | `constant expression expected` |
| union member write-then-read | `U u{}; u.i = 5; return u.i;` | `constant expression expected` |
| bit-field read | `B b{}; b.a = 5; return (int)b.a;` | `constant expression expected` |
| reference member | `R v{x}; return v.r;` | `lvalue expected` - distinct symptom, check whether it shares the evaluator path |

Cases: `test_constexpr_user_provided_constructor.cpp`,
`test_constexpr_union_member_write_read.cpp`, `test_constexpr_bitfield_read.cpp`,
`test_constexpr_reference_member.cpp`.

`constexpr Bad() : b(2), a(b + 1) {}` is rejected today, but only because the
constructor is not evaluated at all. It does not yet prove declaration-order
diagnosis. Re-check once the constructor gap closes: it must still be rejected,
for the right reason. `fail/test_constexpr_constructor_member_order.cpp` holds
that requirement; it passes today and must keep passing after the repair.

## Lexing and literals

No open gap. Two distinct encoding prefixes are already rejected with
`incompatible string literal encodings`, an unprefixed literal adopts its
neighbour's prefix, and `"a" L"b"` is valid wide concatenation that
`features/Expressions/pass/test_mixed_string_sequence_encodings.cpp` already
asserts. The earlier entry here described valid code as if it were a defect.
`fail/test_incompatible_literal_prefix_concatenation.cpp` guards the rejection.

## Core C++17

- class template argument deduction works only from braced initializers;
  `W w(3);` gives `class template argument deduction requires a braced
  initializer`
- `std::is_copy_assignable` is missing (`'__cpc_ns_std_is_copy_assignable'
  undeclared`)

Cases: `test_ctad_parenthesized_initializer.cpp`,
`test_is_copy_assignable.cpp`.

## Pack expansion

No open gap. A pack expansion whose argument is a pattern around the pack
(`Tup<typename decay<Ts>::type...>`) expands in a template-id argument list,
in a function's return type and in a local declaration; the body-level pattern
boundary that truncated `Tup<...>` to the enclosing `Tuple` name is fixed.
`test_pack_expansion_in_return_template_id.cpp` retains that.

A non-type pack element as an explicit template argument (`at<I>()...`,
`std::get<I>(t)...`) also compiles and runs, and has left this queue.

Deduction from a parameter type (`void f(box<Ts...>)`), a pack deduced from two
positions, and plain `f(v...)` expansion have always worked.

What still keeps `make_tuple`, `tie` and `apply` spelled out to six parameters
is the *call* expansion over a forwarding pack inside the body
(`return result_type(std::forward<Ts>(values)...)`, reduced case
`return result_type(static_cast<Ts &&>(values)...)`): it fails with `fold
expression requires an unexpanded parameter pack`, and the variadic constructor
template that would receive the arguments reports `constructor target is not
declared as function`. `include/runtime/tuple` keeps its 1..6 overloads, and
seven-argument `make_tuple` still fails (`test_cpp17_tuple_high_arity.cpp`).
Collapsing them is the payoff for that fix, not a separate gap.

## Libraries

`include/runtime/variant` is a hand-written four-parameter template
(`class A, class B = __variant_empty, class C, class D`), which is the root of
most of this list:

- more than four alternatives: `template 'variant' expects 1 type argument,
  got 5`
- converting construction: `std::variant<int, double, char> v{2.5}` gives `no
  matching constructor ... with 1 list-initializer elements`; only
  default-construct-then-assign works
- `get<Index>`: absent, only `get<T>` exists
- `get_if`: absent
- `emplace`: absent
- non-copyable alternative assignment: unreachable until construction works

Cases: `test_variant_more_than_four_alternatives.cpp`,
`test_variant_converting_construction.cpp`, `test_variant_get_by_index.cpp`,
`test_variant_get_if.cpp`, `test_variant_emplace.cpp`,
`test_variant_noncopyable_assignment.cpp`.

`std::get<double>(tuple)` (get by type) fails inside `include/runtime/tuple`.
Case: `test_tuple_get_by_type.cpp`.

`std::optional` is unusable in constant expressions (`constexpr
std::optional<int> o{5}` gives `constant expression expected`), and
`std::is_copy_constructible<std::optional<NC> >` is wrongly true.
Cases: `test_optional_constexpr.cpp`,
`test_optional_copy_constructible_trait.cpp`.

## Diagnostics

- `(t + 1 + ...)` is accepted although fold operands must be cast-expressions

Case: `fail/test_fold_operand_cast_expression.cpp`.

## Closed and covered

Re-checked as working on 2026-09-16, now with retained coverage. No compiler
work left in this list:

- non-type pack element as an explicit template argument (`at<I>()...`,
  `std::get<I>(t)...`) - `test_explicit_template_argument_from_pack.cpp`
- user-defined literals constant-evaluate (`2.5_km == 2500.0L`)
  - `features/Expressions/pass/test_cooked_numeric_literal_types.cpp`
- encoded-literal sizes match the Microsoft x64 ABI: `sizeof(u"ab")` 6,
  `sizeof(U"ab")` 12, `sizeof(L"ab")` 6, `sizeof(u'a')` and `sizeof(L'a')` 2,
  `sizeof(U'a')` 4 - `features/Abi/pass/test_msvc_encoded_literal_sizes.cpp`
- a `constexpr` function returning the address of a function-local object is
  rejected - `features/Templates/fail/test_constexpr_local_array_pointer_escape.cpp`
- selection and loop scopes, subobject mutation, initialization order,
  references, pointer bounds and discarded runtime calls in constant
  expressions - the `features/Templates/fail/test_constexpr_*` family
- folds over non-type packs - `features/Templates/pass/test_variable_template_pack_expansion.cpp`
- constexpr array element reads inside a fold
  - `features/Templates/pass/test_cpp17_fold_constexpr_array_element_read.cpp`
- a thirty-element fold - `features/Templates/pass/test_cpp17_fold_thirty_elements.cpp`
- rejection of an empty unary fold - `features/Templates/fail/test_cpp17_fold_empty_add.cpp`
- a dependent trailing return type whose `decltype` names a dependent call
  - `features/Templates/pass/test_trailing_return_overload_substitution.cpp`

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
  boundary. Publication does not need the pedantic tier: `scripts\build.exe`
  runs `-Regression`, which is the gate.
