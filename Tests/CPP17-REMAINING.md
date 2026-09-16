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
exactly the number of open gaps: 4 cases, all in `pass/`. That count is the
work list, and it falls as gaps close.

The open-gap cases are the whole of `Tests/tiers.json`'s fast list, so the
routine loop reports the work list instead of hiding it until the pedantic run,
and a green fast run means the queue is empty. Fast is ~0.2s; pedantic carries
every representative case. A fixed case leaves `fast` by not being listed, and
returns only if the routine loop needs that behavior covered on every pass.

Each case names its facility, so a fix starts by running it:

```
Tests\test.exe -Suite features/Cpp17Gaps -Select test_constexpr_reference_member.cpp
Tests\test.exe -Suite features/Cpp17Gaps
```

## constexpr object model

Two symptoms of the one evaluator remain. A local constexpr object of a class
type whose constructor is user-provided is not constant-evaluated at all, and
a reference member cannot be read through.

| Gap | Reproducer shape | Observed |
| --- | --- | --- |
| user-provided constructor is not constant-evaluated | `constexpr S(int v) : a(v) {}` then `static_assert(S(1).a == 1)` | `constant expression expected` |
| reference member read | `R v{x}; return v.r;` | `lvalue expected` for `R v{x}`, `cannot convert 'int &' to 'int'` past it |

The reference member needs the evaluator's own member read: binding is
expression-shaped, so a one-element braced list whose first member is a
reference folds the referent away before the binding sees it, and a reference
member read has to keep referring to the referent object rather than its
stored address.

Cases: `test_constexpr_user_provided_constructor.cpp`,
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

## Pack expansion

No open gap. A pack expansion whose argument is a pattern around the pack
(`Tup<typename decay<Ts>::type...>`) expands in a template-id argument list,
in a function's return type, in a local declaration and in a namespace-scoped
class template; the body-level pattern boundary that truncated `Tup<...>` to
the enclosing `Tuple` name is fixed. `test_pack_expansion_in_return_template_id.cpp`
retains that.

A non-type pack element as an explicit template argument (`at<I>()...`,
`std::get<I>(t)...`) also compiles and runs.

The call expansion over a forwarding pack
(`return result_type(static_cast<Ts &&>(values)...)`) also works, so
`make_tuple`, `tie` and `apply` in `include/runtime/tuple` are one pack
expansion each. Three defects stood in the way, all repaired: a variadic
constructor template with an unnamed parameter pack declared a single
parameter (`constructor target is not declared as function`), the fold
detector claimed a nested declaration's `U &&...` as a fold of the enclosing
template's pack (`fold expression requires an unexpanded parameter pack`), and
the pattern scan only recognised a class template name in the global
namespace, so a namespace-scoped `Tuple<...>` pattern absorbed its own head.
`test_cpp17_tuple_high_arity.cpp` retains seven-argument `make_tuple`, `get`
and `apply`, and `test_variadic_constructor_pack.cpp` the unnamed constructor
parameter pack the reduced shape needs.

Deduction from a parameter type (`void f(box<Ts...>)`), a pack deduced from two
positions, and plain `f(v...)` expansion have always worked.

## Substitution of template-ids that spell a pack

No open gap. A qualified template-id whose argument list spells a pack now
expands in a function's parameter type, in its return type and in a replayed
body, and a partial specialization whose argument list ends in `>>` now
substitutes the alias argument. Retained cases are listed under closed work.

## Libraries

`std::optional` is unusable in constant expressions (`constexpr
std::optional<int> o{5}` gives `constant expression expected`), and
`std::is_copy_constructible<std::optional<NC> >` is wrongly true. The second
answer needs the trait to see a copy constructor that is *deleted* for a
non-copyable element, which this `optional` (a plain user-provided copy
constructor) cannot give: the storage has to be a union whose copy operation
is defaulted behind a conditionally deleted base. The first then follows the
constructor rule above, since `optional<int> o{5}` constructs through
`emplace`.

Cases: `test_optional_constexpr.cpp`,
`test_optional_copy_constructible_trait.cpp`.

Not from the queue but red in the retained set: a function template address as
a template argument (`run_char<record>()` for
`template<void (*F)(char)> void run_char()`) fails with `no matching function
template 'run_char'`. Reproducer:
`Tests/test.exe -Suite features/Templates -Select
test_function_template_address_argument.cpp`. Red at the pre-queue commit as
well, so it is a gap the queue never listed.

## Closed and covered

Re-checked as working on 2026-09-16, now with retained coverage. No compiler
work left in this list:

- a pack expansion as a template argument of a qualified template-id in a
  parameter type - `test_qualified_template_id_pack_argument.cpp`
- a template-id that spells a pack inside a replayed function body, and a
  partial specialization whose pattern deduces the pack -
  `test_pack_template_id_in_replayed_body.cpp`,
  `test_tuple_get_by_type.cpp`
- a partial specialization whose argument list ends in `>>`
  (`helper<T, void_t<T &>>`) - `test_partial_specialization_alias_argument.cpp`
- union member write-then-read in a constant expression, while a read of an
  *inactive* union member stays rejected
  - `test_constexpr_union_member_write_read.cpp`,
    `features/Templates/fail/test_local_constexpr_inactive_union_read.cpp`
- bit-field write-then-read in a constant expression
  - `test_constexpr_bitfield_read.cpp`
- a variadic `std::variant` (more than four alternatives, converting
  construction, `get<Index>`, `get_if`, `emplace`, non-copyable alternative
  assignment) - `test_variant_*.cpp`
- `std::get<T>(tuple)` - `test_tuple_get_by_type.cpp`

- non-type pack element as an explicit template argument (`at<I>()...`,
  `std::get<I>(t)...`) - `test_explicit_template_argument_from_pack.cpp`
- assignment to a class that only declares a move assignment operator is
  rejected, and `std::is_assignable`/`is_copy_assignable`/`is_move_assignable`
  answer from the same rule - `test_is_copy_assignable.cpp`
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
- a fold operand that is not a cast-expression (`(t + 1 + ...)`) is rejected
  - `fail/test_fold_operand_cast_expression.cpp`
- class template argument deduction from a parenthesized initializer
  - `test_ctad_parenthesized_initializer.cpp`

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
