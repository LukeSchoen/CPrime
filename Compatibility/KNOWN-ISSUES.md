# Known CPC issues

Open defects, each with its reduced shape and the work it needs. Completed work
is code and retained cases; nothing here is a progress log, so a floor that is
closed leaves this file and lives on as its retained case.

The defects here were found by compiling consumer projects that live outside
this repository. Those checkouts are deliberately not named and their paths are
not written down: reproduce the described shape as a local case in this
repository, from whatever consumer tree a machine keeps.

## Conditional expression whose taken branch is a file-scope const

Closed.  A conditional whose result is an lvalue joins its arms as addresses,
and the join ran the ordinary value conversion on each arm first.  That
conversion reads a named integral constant as its value
(`constexpr_read_local_value`), so a file-scope `const` arm stopped being an
object: the shared indirection under the join then loaded *through the
constant* (`size <= limit ? page_size : size` dereferenced 32704) instead of
through the arm's address.  `expr_cond` now marks each arm whose lvalue the
join needs (`cpp_conditional_lvalue_arm`) while it applies the common type, and
the value fold leaves such an arm alone.

The defect was wider than the crash: the same shape silently answered with the
wrong object when the conditional was bound to a reference
(`&pick(true) != &page_size`,
`Compatibility\build\cond-ternary\cases\reference_identity.cpp`).

```cpp
/* cpc.exe -o tx3.exe tx3.cpp  ->  runtime error: invalid memory access */
#include <cstdio>
#include <cstddef>
static const size_t page_size = 32704;
static size_t f(size_t size) { return size <= 8000 ? page_size : size; }
int main() { printf("%llu\n", (unsigned long long)f(64)); return 0; }
```

Equivalent spellings that work, and did before: an `if` statement (`tx2.cpp`),
a macro constant (`tu2.cpp`), a literal arm, or a non-`const` global (`tw1.cpp`,
`tw2.cpp`).  The reduction is `Compatibility\build\top2\src\tx3.cpp`.

Consumer impact: a consumer's server entry point crashed right after start,
because pugixml's `allocate_memory_oob` uses exactly this shape
(`size <= threshold ? xml_memory_page_size : size`); the first node allocation
inside an XML parse faulted.  The consumer's temporary workaround for that call
site (the ternary written as an `if`) is reverted: its `cpc.exe` copy was
refreshed to this tree's published compiler (SHA-256 hashes match), the library
was rebuilt, and the rebuilt program now loads its content and runs with an
empty stderr (`Compatibility\build\cond-ternary\verify-revert.log`).

Evidence: `Compatibility\build\cond-ternary\`.  `evidence-before.log` and
`evidence-after.log` are one eight-case matrix run against the retained pre-fix
compiler (`cpc-before.exe`, the published `cpc.exe` of cycle 21, SHA-256
`FF8146DABCAAC728B2F436CDF5D42473AD8FA590311EE622C9F499172E1AA7F5`) and the
published one (`B5212CB89826790E51C80EFC992A722AE2508FC2F147E72EDDF714942D6C8A53`):
the `const`, `constexpr` and external-`const` arms crash with exit `-1073741819`
and the reference case answers `named=0` before the fix, while the `if`
statement, literal, non-`const` global, assignment and control cases are green
in both.  The `-S` listing the first report read is itself defective (next
entry), so the object code was read back with `...\pe_code.py`; the pugixml
shape `Compatibility\build\top2\src\oob_ternary.cpp` now prints
`wanted=32704`.  Retained as
`features/Expressions/pass/test_conditional_lvalue_constant_arm.cpp`; the
published checks were features/Expressions 231, features/Classes 248,
features/Constructors 214, features/Templates 971, features/StdConcurrency 15,
features/Includes 76, fast 29 and `-Regression` 66.

## `-S` drops instructions whose operand carries a symbol relocation

Open, pre-existing, and found while reading the conditional-expression code.
`cpc -S` prints the backend's instruction trace (`s1->asm_text`, copied out by
`cprime_output_asm`), not the emitted section, and the emission path for a
PC-relative symbol reference never appends its mnemonic to that trace, so the
instruction vanishes from the listing together with its opcode bytes.

```c
int x = 5;
int main(void) { return x; }   /* runs to exit 5 */
```

```
$ cpc.exe -S -o g.s g.c   main:  push rbp, mov rsp->rbp, sub rsp, leave, ret
$ pe_code.py g.exe        main:  push rbp, mov rsp->rbp, sub rsp,
                                 mov eax, [rip + 0x1fef], leave, ret
```

The observation is `Compatibility\build\cond-ternary\findings.log`, the
reductions are `...\cases\global_read.c` (the load is missing) and
`...\cases\local_read.c` (a local keeps its load), and `...\pe_code.py` reads
the object bytes.  This is why the first conditional-expression report read as
loads of locals that were never stored.

Work: emit the relocating instructions into the trace as well, or print the
section bytes through a disassembler, retain one minimal case in
`features/Expressions`, and run the affected suite, `-All -Tier fast`,
`-Regression` and `src\scripts\build.exe`.

## A diagnostic raised while replaying another file's tokens

Open, pre-existing, and found while closing the 18 September priorities.  The
compiler prints the file being compiled with whatever line number the innermost
saved-token replay last set.  When that replay carries tokens from another file
the pair disagrees: the pre-fix `std::thread` failure (see its closed entry
below) reported line 17 of a three-line translation unit, and line 17 belonged
to the runtime `<thread>` header; inside a large translation unit that looks
like an unrelated line of the file being compiled.

Reduction: `Compatibility\build\top2\cases\th1_voidptr.cpp` recorded the shape
before the runtime repair, and `Compatibility\build\top2\cases\diag2.cpp` with
`diag_h.h` still replays a header body today.  A repair needs a per-token-string
origin file: `begin_macro`/`end_macro` keep only line numbers, while `file`
always names the caller.

## Member-pointer types nested in a function type used as a class template argument

Open, pre-existing, and recorded here because the 18 September probe matrix
covered member-pointer parameters.  A member-pointer or
pointer-to-member-function type nested inside a function type that is itself a
class template argument still stops with `native linkage type nesting too deep`
or `static data member name`.

Reductions: `Compatibility\build\top2\cases\mp4.cpp`
(`std::function<bool(int methods::*, float)>` in a plain struct, no templates)
and `Compatibility\build\top2\cases\mp3.cpp` (the same shape with
`bool (methods::*)(int &, float)`).  The bare alias spellings of the same
parameter types compile, as do `std::function` function types whose parameters
are ints or pointers.

## A class template's own member typedef used inside another member alias

Open, pre-existing.  When a class template declares a typedef and uses it inside
another member alias of the same class, the alias replay reports
`using alias type expected`; a class-scope nested type in the same position
behaves the same way, while a namespace-scope typedef in the same position
works.

Reductions: `Compatibility\build\top2\cases\k04_member_typedef.cpp`
(`using H = std::function<bool(R, float)>` with `typedef int &R;` declared in
the same class template) and
`Compatibility\build\top2\cases\w13_qualified_member.cpp` (a nested typedef of
another class as the first parameter).

## A nested same-type temporary claimed the initializer's construction destination

Closed.  A functional-constructor call that cannot use the pending construction
destination (`Path(Str())` where the object being initialized is a `Str`) left
that destination visible to its own arguments.  A nested same-type temporary
then claimed the destination's storage, so it was built as a named object
instead of a prvalue, and the enclosing call's reference parameter rejected it
with `rvalue reference cannot bind to an lvalue`.  That is the shape a nested
same-type temporary in a functional-constructor call hits, and it stopped a
consumer's server build once the 18 September priorities were published.

`try_parse_cpp_functional_constructor` now hides the pending destination while
it evaluates a call that cannot use it and restores the destination afterwards,
and a substitution probe hands the destination back with the availability it
was given instead of leaving it consumed.

Evidence: `Compatibility\build\top2\cases\` (`rva.cpp` and `rvl.cpp` are the
reductions; `cp1.cpp` and `cp3.cpp` compile the same expression against the
consumer's headers).  The pre-fix compiler reports the error and the published
one compiles all of them.  Retained as
`features/Constructors/pass/test_nested_temporary_argument_keeps_destination.cpp`.

The consumer needed one source fix as well: a setter took `v2I &` while the
call passes a prvalue, so the parameter is `const v2I &` now.  Both the pre-fix
and the published compiler correctly reject the original call, and with the fix
the consumer's full build completes.

## Pack expansion into a function type: the parameter list starts at its first entry

Closed.  A pack expansion inside a function type is a parameter list, so its
expanded parameters may begin with any type; the "leading reference" in the
report was the visible edge of two frontend defects, not the rule.  A leading
pointer, `const`-qualified type or class-type argument failed the same way, and
so did the bare `using Handler = bool(Args...);` spelling.

- `template_paren_starts_parameter_list`, the lookahead that keeps
  `std::function<bool(Args...)>` a function type instead of a functional cast,
  accepted only builtin type keywords.  A pack element that is a reference,
  pointer, `const`-qualified or class type arrives as a generated type name, so
  `bool(first, rest)` read as `bool(expression)` and closed at the first
  comma.  The helper now accepts every declaration-specifier start
  (`token_can_start_parameter_declaration`) and resolves qualified names
  (`ns::I`, `::I`) while still leaving `bool(std::trait<T>::value)` and
  `bool(flag)` expressions.
- the `using`-alias replay of a class template spelled the alias as
  `typedef <type-id> Name;`, which is not a typedef of a function, array or
  pointer-to-function type.  The declared name now lands in the declarator's
  own name slot (`typedef bool Name(int);`) through
  `template_abstract_declarator_name_index`, so `using F = bool(int);`,
  `using A = int[4];` and `using P = bool (*)(int, int);` in a class template
  all declare what they say.

Evidence: `Compatibility\build\top2\evidence-after.log` (the whole probe
matrix, the consumer-shaped repro and the `std::thread` matrix against the
published compiler), with the reductions and probe cases in
`Compatibility\build\top2\cases\`.  Retained as
`features/Templates/pass/test_function_type_pack_expansion_parameter_list.cpp`
and
`features/Templates/pass/test_class_template_function_type_alias_declarator.cpp`.
A consumer header's workaround (the function-typedef spelling) can revert: an
event type whose expansion starts with a reference now compiles, and the
typedef spelling keeps working.

## std::thread with a bound pointer argument

Closed.  The argument storage of `src/include/runtime/thread` passed each bound
argument on as a forwarding reference (`Bound&&... bound`) before handing it to
`std::invoke`.  When the stored member's type was a substituted pointer type
(`void *`, `int *`) the frontend bound that forwarding reference to the
enclosing class instance instead of the pointer, so the `std::invoke` call
inside `__cpc_thread_arguments<>::invoke` found no candidate and reported
`no matching function template '__cpc_ns_std_invoke'`.  The base case now takes
its bound arguments by value and moves them into `std::invoke`, which is the
same observable behaviour (`std::thread` passes its stored copies as rvalues)
without the shape that misdeduced.  Both the reported shape and the
pointer-to-member-function shape go through this one path.

Evidence: `Compatibility\build\top2\evidence-after.log` and the
`th1..th8` matrix in `Compatibility\build\top2\cases\`: with the published
compiler before the fix `std::thread t(f, (void*)0)`,
`std::thread t(g, (int*)0)`, `std::thread t(lambda, p)` and
`std::thread t(f, 1, (void*)0)` were red while `int` arguments, lambdas without
bound arguments and `std::invoke` written directly were green; all eight now
pass.  Retained as
`features/StdConcurrency/pass/test_thread_function_pointer_argument.cpp`,
beside the pre-existing
`features/StdConcurrency/pass/test_thread_deferred_member_address.cpp`, which
this repair also turns green (it was the retained pointer-to-member case).
Consumer: a consumer's server code that starts threads with a bound pointer
argument now compiles unchanged.  The diagnostic and coverage limits this probe
found are the open entries at the top of this file.

## boost/math/special_functions/sign.hpp: parenthesized template declarators
## Pack expansion into a function type whose first parameter is a reference

Open, found while compiling a consumer tree. A pack expansion is accepted in a
function type only while no expanded parameter is a leading reference:

```cpp
#include <functional>
template<typename... Args>
class Event { public: using Handler = std::function<bool(Args...)>; Handler h; };
Event<int&, float> e;

int main() { return 0; }
```

```
$ cpc.exe -c event_pack.cpp -o ...
.../event_pack.cpp:3: error: ')' expected (got ',')
```

The diagnostic is attached to the alias's own line. `Event<int, float>`,
`Event<int, float&>` and `Event<int, float, double>` all compile, and a bare
`using Handler = bool(Args...);` fails the same way, so the defect is the
frontend's scan of a pack expansion into a function type's parameter list, not
the runtime header. The equivalent function-typedef form is accepted
(`typedef bool Signature(Args...); using Handler = std::function<Signature>;`),
which is what the consumer now uses; the alias form and the direct form are
what has to start working.

Consumer: an events header, where a class template declares a member of the
shape `Event<Observer &, T &&>` and explicit specializations at the end of the
header instantiate it, so any translation unit that includes the header fails.

Work: reproduce with root `cpc.exe`, retain a minimal case in
`features/Templates/pass/` (a reference-first pack inside a function type),
repair the shared pack-expansion/function-type scan, run the affected suite,
`-All -Tier fast` and `-Regression`, publish with `src\scripts\build.exe`, and
tell the consumer side that the header workaround can revert.

## std::thread with a function pointer taking a pointer

Open, found while compiling a consumer tree, and reached through the same
runtime chain as the pointer-to-member case below.

```cpp
#include <thread>
void f(void *p) {}
int main(){ std::thread t(f, (void*)0); t.join(); return 0; }
```

```
error: no matching function template '__cpc_ns_std_invoke'
```

`std::invoke(f, (void*)0)` written directly matches, a lambda target
(`std::thread t([]{}, p)` with `void *p`) compiles, and `std::thread t(f, 1)`
with an `int` parameter compiles, while `void *` and `int *` parameters both
fail. The diagnostic's line number is stale (it named line 17 of a three-line
file), so in a large translation unit it looks like an unrelated include line.

Consumer: a consumer's server code that starts threads with a bound pointer
argument, in several translation units.

Work: reduce it next to the pointer-to-member case (both reach
`__cpc_thread_arguments<>::invoke` -> `std::invoke` in
`src/include/runtime/thread`), repair that path once for both shapes, retain one
minimal case per shape, run the affected suite, `-All -Tier fast` and
`-Regression`, then publish with `src\scripts\build.exe`.

## boost/date_time/gregorian/gregorian_io.hpp: repeated Entry class definition

Open, and the current consumer floor. The current probe now reaches
`Compatibility\build\consumer-probe46.log` and stops in
`boost/date_time/gregorian/gregorian_io.hpp:220` with `struct/union/enum
'__cpc_ns_std_multimap__char____cpc_ns_boost_date_time_string_parse_tree__char____cpc_ns_std_less__char____cpc_ns_std_allocator____cpc_ns_std_pair____cpc_template_type_const_char____cpc_ns_boost_date_time_string_parse_tree__char_Entry'`
already defined.

Work: reduce it to a standalone case under `Compatibility\build`, repair the
shared template/member-class mechanism, retain one minimal case, then run the
affected suite, `-All -Tier fast`, `-Regression` and `src\scripts\build.exe`
before probing the consumer again.

## std::thread with a pointer-to-member function is rejected

Open, pre-existing, and outside the fast list and the gate -- the same standing
the two `<charconv>` `std::errc` probes have. The retained case
`features/StdConcurrency/pass/test_thread_deferred_member_address.cpp` is red in
the merged tree, and the same failure reproduces with the current tree's
compiler changes reverted, so it came in with an earlier merge: the base copy's
own `cpc.exe` (`C:\Luke\Src\PRIME\CPrime`, 17/09 21:45) still compiles the case,
and the case file is byte-identical
(sha256 `E39B856397B392C23457E936A91B43321A11791CFAF2535759666B6AE86FA247`).

The reduction is `Compatibility\build\eval-if-floor\invoke\v1_thread_member_pointer.cpp`:

```cpp
struct Worker { void task(); };
void Worker::task() {}
int main()
{
  Worker w;
  std::thread t(&Worker::task, &w);
  t.join();
  return 0;
}
```

```
$ cpc.exe -c Compatibility\build\eval-if-floor\invoke\v1_thread_member_pointer.cpp -o ...\v1.obj
Compatibility/build/eval-if-floor/invoke/v1_thread_member_pointer.cpp:17: error: no matching function template '__cpc_ns_std_invoke'
```

`std::invoke(&Worker::task, &w)` written directly matched, so the gap was in the
runtime's `std::thread` constructor path (`src/include/runtime/thread:16-26`,
`__cpc_thread_arguments<>::invoke` -> `std::invoke` with a `std::move`d
argument), not in `std::invoke` itself.  The four-probe matrix and the exact
commands are in `Compatibility\build\eval-if-floor\invoke\invoke-floor.log`, and
the repair's own evidence is `Compatibility\build\top2\evidence-after.log`.
pointer-to-member), not in `std::invoke` itself. The four-probe matrix and the
exact commands are in `Compatibility\build\eval-if-floor\invoke\invoke-floor.log`.

## A parenthesized initializer from a functional cast over `new T()` / `new T`

Open, found while reducing the `foreign_ptr.hpp` floor. `W w(W(new int()));`
is an object definition: the parenthesized content cannot be a parameter
declaration, because `new` cannot be a declarator-id, so the declaration
heuristic has to fall back to the initializer. It instead takes `new` for the
parameter name and reports `')' expected (got 'int')`
(`Compatibility\build\foreign-ptr\declarator_functional_cast_new.cpp:13`).
The same shape with a non-empty new-initializer (`W w(W(new int(3)));`,
`Compatibility\build\foreign-ptr\q1.cpp`) is accepted, so the two paths
disagree. Both results are in
`Compatibility\build\foreign-ptr\discovered-gaps.log`.

## An assignment operator reached through a conversion from a same-class temporary

Open, found while reducing the `foreign_ptr.hpp` floor. A class that declares
a copy assignment operator which cannot bind a temporary (`A& operator=(A&)`)
and another assignment operator whose parameter needs a user-defined
conversion (`A& operator=(Ref)`) has to convert the temporary for
`a = A();`. Overload resolution drops the converted candidate because the
operand's class matches the receiver, and reports `no matching user-declared
copy assignment operator`. `std::auto_ptr`'s `operator=(auto_ptr_ref<T>)` is
the standard-library instance of the shape. Reproducer:
`Compatibility\build\foreign-ptr\assignment_from_converted_rvalue.cpp`; the
exact result is `Compatibility\build\foreign-ptr\discovered-gaps.log`.

## boost::mp11 member alias templates

Open, but not the current consumer stop. A member alias template declared
inside a class template is not registered -- `boost::mp11::mp_quote<P>::fn`
reports `nested template type member '...::fn' must be a typedef`. Reductions:
`Compatibility\build\mp11-floor\mp_quote_probe.cpp` and `member_alias.cpp`.
A second shape fails even with that lookup present: when several
specializations of one class template each declare the alias, the name is
registered once per specialization and a later one rebinds the body
(`ns_alias_tt3.cpp`, `tmpl_tmpl_alias3.cpp`). The member alias needs to be
registered per class-template specialization, keyed by the instance token,
before the consumer can proceed through the remaining Boost.Parameter/MPL
headers.

## boost::date_time: `date` through its base-class initializer

Open. `boost::gregorian::date` alone reports `new requires a viable default
constructor for '...date_time::date<...gregorian_date...>::type'` when a `date`
is constructed; the diagnostic is misleading, since
`boost::gregorian::date::duration_type` alone constructs. The `date` class
template instantiated through `class date : public
date_time::date<date, gregorian_calendar, date_duration>` does not build, and
its constructor is the one whose base initializer names `date_time::date<...>`
in the normal template-id spelling.

Work: reduce it to a local case, then make the replayed base-class initializer
resolve the class-template instantiation to the base subobject.

A base-initializer name that is a class-scope typedef (`typedef Inner base_type;
... : base_type(c)`) is already closed for the direct case: the class alias
tables are consulted, so the typedef-named base initializer attaches to the
base subobject. It needs no new retained case; its consumer spelling is still
blocked behind this floor.

## A process-list consumer: psapi.h

`psapi.h` is absent from `third-party/win32-sdk/include`; the installed Windows
SDK ships `um/Psapi.h`.

Work required:

- Choose the packaging shape for the missing SDK header and add it.
- Compile and run the uses the project makes of the process API.

## Recorded leads: inference kernels (a read-only consumer tree)

Convolutions dominate inference at roughly 85% of the time, and a hand-written
SSE GEMM measured about 5x faster than the C loop in isolation
(`KPOSE_GEMM=sse` 175.8 ms against 326.3 ms for the scalar kernel on the
verification model; single-threaded A/B is forced by an environment switch).
Both blocking defects are closed and retained: the atomic work counter returns
its result
(`features/Abi/pass/test_msvc_interlocked_counter.cpp`) and the inline-SSE
kernels build and verify
(`features/GnuExtensions/pass/test_inline_sse_asm_with_scalar_tail.cpp`). The
external reproducer and its notes live in that checkout, which is read only and
is not named here.
