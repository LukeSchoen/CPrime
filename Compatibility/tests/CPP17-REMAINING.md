# C++17 remaining scope

Working queue only: open gaps and the rules for closing them. Completed work is
code, retained cases are the record of what passes, and git is the record of
what changed. Root `cpc.exe` is the only compiler, one process at a time.

## Test loop

```
Compatibility\tests\test.exe -All -Tier fast        the loop: the open gaps below, ~0.2s
Compatibility\tests\test.exe -Suite features/X      one suite, every retained case
Compatibility\tests\test.exe -Regression            publication gate
src\scripts\build.exe                 self-host, validate, publish cpc.exe
```

Do not run the pedantic tier. It is close to banned: run it only as the last
and only step of an important confirmation, and avoid it if at all possible.
The affected suite plus `-Regression` is the broad check.

`Compatibility/tests/tiers.json` lists the fast subset, which is the open work list; every
other retained internal case belongs to the pedantic tier, so a case leaves the
loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## Open gaps

`Compatibility/tests/tiers.json` holds one fast-tier case per gap that is still
red, so a green fast run means the queue is empty. The fast list is empty:
every floor closed so far is retained as a passing case.  The braced functional
cast in `boost/mp11/algorithm.hpp:251` is retained as
`features/Templates/pass/test_braced_functional_cast_template_argument.cpp`;
the explicit-specialization absolute-base case is retained as
`features/Templates/pass/test_explicit_specialization_absolute_base.cpp`; the
qualified-self case is retained as
`features/Templates/pass/test_qualified_self_static_member.cpp`; and the
`named_scope.hpp` floor is retained as
`features/Includes/pass/test_reverse_iterator.cpp`,
`features/Includes/pass/test_allocator_member_types.cpp`, and
`features/Classes/pass/test_out_of_class_constexpr_static_array.cpp`; the
`tuple_basic.hpp` relative-namespace owner floor is retained as
`features/Classes/pass/test_pointer_to_member_relative_namespace.cpp`.  The
`boost/serialization/serialization.hpp` `check_is_complete` floor is retained
as
`features/Templates/pass/test_dependent_class_template_base_constant.cpp`.
The `boost/bind/placeholders.hpp` constant-initialization floor is retained as
`features/Templates/pass/test_constexpr_default_initialized_template_object.cpp`.
The consumer now clears `boost/parameter/aux_/arg_list.hpp`,
`boost/log/keywords/severity.hpp`,
`boost/log/attributes/named_scope.hpp`, and
`boost/tuple/detail/tuple_basic.hpp`,
`boost/serialization/serialization.hpp`,
`boost/bind/placeholders.hpp`, and `boost/parameter/aux_/set.hpp`
(`boost::mp11::mp_list<>` used to resolve to an empty instance of the template
instead of the template; retained as
`features/Templates/pass/test_empty_instantiation_keeps_template_name.cpp`),
then clears `boost/move/detail/type_traits.hpp` (the leading alignment attribute
of a partial specialization used to end the class-key context, so the second
`aligned_struct<Len, N>` was rejected as a redeclaration; retained as
`features/Templates/pass/test_alignment_attribute_partial_specialization.cpp`),
and now clears `boost/optional.hpp`.  The global `boost_optional_detail`
namespace and nested `boost::optional_detail` used to collide in the scope
table; keeping one record per distinct lexical path fixes the depth underflow,
and the standalone case is retained as
`features/Namespaces/pass/test_underscore_namespace_path_collision.cpp`.
The consumer cleared `boost/signals2/detail/foreign_ptr.hpp` once the runtime
`<memory>` kept the C++98 `auto_ptr` that `boost::scoped_ptr`'s
`BOOST_NO_AUTO_PTR` guard leaves in the parse; that floor is retained as
`features/Includes/pass/test_auto_ptr_transfer_ownership.cpp`.  It then cleared
the `boost/signals2/slot_base.hpp` chain's lower floors: the value-hash
deduction pattern (`std::basic_string<Ch, std::char_traits<Ch>, A>`, whose
trailing parameter stayed unbound when a concrete argument preceded it), the
missing `std::basic_string_view` traits parameter, and the `numeric_limits`
member set that `boost/container_hash/detail/hash_float.hpp` selects its binary
hash with.  They are retained as
`features/Templates/pass/test_template_deduction_after_concrete_argument.cpp`,
`features/Includes/pass/test_basic_string_view_traits_parameter.cpp` and
`features/Includes/pass/test_numeric_limits_standard_members.cpp`, with the
reductions and before/after commands in
`Compatibility\build\signals2-probes\reduce\deduction-fix.log` and
`Compatibility\build\runtime-header-floors.log`.  It then cleared the dependent
base clause of `boost/mpl/eval_if.hpp:40`,
`struct eval_if : if_<C,F1,F2>::type`: the replay's `template-id :: member`
path asked for the qualifier's identity while the member-alias deferral was
set, so `if_<...>` stayed a forward declaration and its member `type` was never
declared.  Retained as
`features/Templates/pass/test_base_member_of_template_id.cpp`; the reduction
is `Compatibility\build\signals2-probes\reduce\q1_eval_if_class_bases.cpp` and
the commands, the before/after consumer runs and the second defect found while
validating are in `Compatibility\build\eval-if-floor\floor-18.log`.  The
consumer now stops at `boost/signals2/slot_base.hpp:40` one level deeper, where
the condition of `if_`/`eval_if` is a member class of the specialization that
demands the instantiation; the reduction is
`Compatibility\build\eval-if-floor\next\reduce_boost_shape.cpp` and its exact
command, result and trace are in
`Compatibility\build\eval-if-floor\next\next-floor-reduce.log`.  The next action
is to make that deferred instantiation wait for the member class argument, then
re-run the Templates suite, the fast tier, `-Regression`, the build, and the
consumer probe.  Two further local shapes found while reducing the closed floor
(a parenthesized initializer from a functional cast over `new T()`/`new T`, and
an assignment operator reached through a conversion from a same-class
temporary) are reproduced under `Compatibility\build\foreign-ptr\` and recorded
in `Compatibility\KNOWN-ISSUES.md`, which holds the detailed open floors.  The
same list holds a pre-existing red retained case found by running the remaining
suites against the merged tree,
`features/StdConcurrency/pass/test_thread_deferred_member_address.cpp`
(`std::thread` with a pointer-to-member), with its reduction under
`Compatibility\build\eval-if-floor\invoke\`.  It is not in the fast list either,
so the two `<charconv>` probes, that case, the floor above and the two local
shapes are the open work the fast list does not yet carry.

To add a gap: add one minimal case under the suite that owns the behavior, list
it in the fast list, reproduce it with root `cpc.exe`, repair the shared
mechanism, then retain the case and drop it from the fast list. A crash
reproducer starts in a suite of its own so one crash cannot abort a shared
compile batch.

External CPC bug reports that are not part of the C++17 queue are recorded in
`KNOWN-ISSUES.md`.

## Where the cases live

- `features/Abi/pass` - Microsoft x64 layout, nullptr and record-return facts
- `features/Cpp17Gaps/pass` - minimal standalone reproducers, one per gap;
  every case is a valid program that must compile, link and exit 0
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
  boundary. Publication does not need the pedantic tier: `src\scripts\build.exe`
  runs `-Regression`, which is the gate.
