# Known CPC issues

Open defects, each with its reduced shape and the work it needs. Completed work
is code and retained cases; nothing here is a progress log.

## boost/math/special_functions/sign.hpp: parenthesized template declarators

Closed.  The declaration scan now recognizes `T (f)(args)` before registration
and rewrites it to the ordinary `T f(args)` token shape, so the declared name
and the signature helpers no longer key the template on its return type.
Retained as
`features/Templates/pass/test_parenthesized_template_definition_then_plain.cpp`;
the failing and passing evidence is in
`Compatibility\build\parenthesized-template-*.log`, and the consumer probe
advanced to the next floor recorded below (`explorer-probe13.log`).

## boost/date_time/period_formatter: `std::ostreambuf_iterator`

Closed.  The runtime `<iterator>` header now declares
`std::ostreambuf_iterator<charT, traits>` with the standard member typedefs,
the streambuf and stream constructors, `operator=`, `operator*`, `operator++`
and `failed()`, so `period_formatter.hpp`'s dependent default argument
resolves.  The definition only needs `<iosfwd>` forward declarations, so
`<iterator>` does not drag in `<ostream>`.  Retained as
`features/Includes/pass/test_ostreambuf_iterator_default.cpp`; the compiler was
republished, and `Compatibility\build\ostreambuf-publish2.log`,
`ostreambuf-includes-suite2.log`, `ostreambuf-fast2.log` and
`ostreambuf-regression2.log` record the passing suite, fast tier and gate.

## boost/date_time: `std::locale::facet`

Open.  With the iterator published, the Explorer++ probe
(`Compatibility\build\explorer-probe14.log`) advances past
`period_formatter` and stops at
`boost/date_time/gregorian/gregorian_io.hpp:49` with
`nested template type member '__cpc_ns_std_locale::facet' must be a typedef`
while the date facets derive from `std::locale::facet`.

Reduced to `Compatibility\build\locale-probes\p1_facet_base.cpp`: a 12-line
program that derives a class from `std::locale::facet` (`p1.log`); the fuller
`p2_facet_on_stream.cpp` adds `os.getloc()`, `imbue` and `has_facet`
(`p2.log`).  The runtime `<locale>` header has no `locale::facet` or
`locale::id`, `locale` has no facet-owning constructor, and the runtime streams
have no `getloc`/`imbue`, so the `date_facet::put` path cannot compile.

Work required:

- Retain one minimal case under `features/Includes/pass`.
- Add `std::locale::facet`, `std::locale::id` and the facet-registering
  `locale(const locale&, Facet*)` constructor.
- Give `has_facet`/`use_facet` real per-type storage instead of the current
  always-default answer, and add `basic_ios::getloc`/`imbue`.
- Compile and run the retained case, then republish and re-run the probe.

## clCRC.cpp: a leading `::` in a replayed member function template

An out-of-class member function template whose declaration and definition spell
a parameter type with a leading `::` (`::G`) is replayed without its declarator,
so the redefinition reports incompatible types (`... versus 'inline unsigned
long long'`). The reproducer is about 20 lines in the clCRC checkout.

Work required:

- Reduce it to a standalone CPrime-local case.
- Keep the declarator when the replay re-spells the parameter type.
- Retain the case in the suite that owns the behavior.

## faceDetectCNN.cpp: SSE and AVX vector types

`__m256` is undeclared: CPrime's `immintrin.h` is a stub, so AVX vector types
are unimplemented. The project also needs the SSE types and intrinsics.

Work required:

- Implement the SSE vector types and intrinsics the project uses, then the AVX
  ones.
- Retain a compile-and-run case for the types and for the intrinsics added.

## clProcessList.cpp: psapi.h

`psapi.h` is absent from `third-party/win32-sdk/include`; the installed Windows
SDK ships `um/Psapi.h`.

Work required:

- Choose the packaging shape for the missing SDK header and add it.
- Compile and run the uses the project makes of the process API.

## RTMPose / Kpose kernels (`C:\Luke\Src\Kinect`, read only)

Convolutions dominate inference at roughly 85% of the time, and a hand-written
SSE GEMM measured about 5x faster than the C loop in isolation, but it cannot
ship while the two defects below are open, so the Kpose kernels stay portable C.
`KPOSE_THREADS=1` forces single-threaded execution for A/B measurements. The
external reproducer and notes are in `C:\Luke\Src\Kinect\README.md`; fixtures
and models are regenerated with `python tools\vendor_rtmpose.py --all`.

### InterlockedIncrement ignores its result

`InterlockedIncrement` returns 0 instead of the new value, so an atomic work
counter never advances.

Reduced shape:

```c
long next = InterlockedIncrement(&job->next) - 1;
/* The loop never observes 1. */
```

Work required:

- Reduce the reproducer to a standalone CPrime-local test.
- Lower the intrinsic to return the post-increment value.
- Run the fixed intrinsic and the retained test in the affected suite.

### Inline SSE asm corrupts surrounding float code

Inline SSE asm using `movups`/`mulps`/`addps` and `xmm0`-`xmm2` corrupts the
surrounding generated float code: a function that runs the asm and then does
scalar float maths crashes with `0xC0000005`. The reduced shape is a GEMM whose
`N=25` crashes while `N=24` and `N=28` do not; `cpc -b` and separate-function
layouts do not avoid it. `__attribute__((vector_size(16)))` works but compiles
about 3x slower than the plain C loop.

Work required:

- Reduce the `N=25` GEMM to a standalone CPrime-local reproducer.
- Fix register allocation and liveness across inline asm and generated float
  code.
- Retain the reproducer as a regression case.
- Re-check the isolated SSE GEMM performance after the fix.

## The standard `std::atomic_*` typedefs crash the compiler

Closed.  The standard typedefs are now in `src/include/runtime/atomic`.  The
crash was a double free while parsing conversion operators:
`make_type_from_saved_type_tokens` consumes its token string, but three
callers released it again.  The pool corruption became fatal once the
`<atomic>` typedef set enlarged the token stream.  Retained as
`features/Atomics/pass/test_atomic_typedefs.cpp`.

## Explorer++ probe: `std::string(begin, end)` from two pointers

The probe is `C:\Luke\Src\Archive\explorerplusplus`, a manifest build of 232
translation units driven by `src\scripts\project.exe`; entry point
`build_cpc.cmd`, reduced cases and their probe in the consumer's
`Scripts\cpc\gaps` and `Scripts\cpc\Test-CpcGaps.ps1`.  Compile it with root
`cpc.exe`, reduce each failure to a minimal local case, and never edit the
consumer.  All five reduced cases in `Scripts\cpc\gaps` compile now.

Closed.  This section is the record of the floor it opened and of the floors
behind it: the runtime `basic_string` gained the standard iterator-pair
constructor, the consumer moved on to
`boost/date_time/gregorian/greg_weekday.hpp`, and every floor it reported
afterwards is closed below.  The retained case is
`features/Cpp17Gaps/pass/test_string_iterator_pair_constructor.cpp`.

### boost/date_time: an inherited member typedef of an instance

Closed.  `class greg_weekday : public greg_weekday_rep` used `value_type`
unqualified in its constructor declarator, and `value_type` came from
`constrained_value<policy>`, a class-template instance.  The instance
registers a nested typedef under the alias token its replayed body declares
(`traits__int__value_type`), which an unqualified lookup inside a class
derived from the instance never consulted.  Retained as
`features/Classes/pass/test_inherited_typedef_from_template_base.cpp`.

### boost/exception: derived-to-base versus `void*`

Closed.  `copy_boost_exception(exception *, exception const *)` competed with
`copy_boost_exception(void *, void const *)` for a `clone_impl<T> *`, and the
pointer tie-break ranked both at the same rank.  [over.ics.rank]/4.4 and
/4.5.1 are now implemented for pointer conversions.  Retained as
`features/OperatorOverloads/pass/test_pointer_conversion_base_over_void.cpp`.

### boost/operators: qualified lookup through a using-directive

Closed.  `boost::less_than_comparable1` is declared in
`boost::operators_impl` and imported into namespace `boost` with
`using namespace operators_impl;`; qualified lookup of a type name never
expanded the qualifier's using-directives, so `boost::date_time`'s base-clause
and template arguments could not be parsed.  Retained as
`features/Namespaces/pass/test_qualified_lookup_through_using_directive.cpp`.

### boost::date_time's `bad_weekday`: exception messages

Closed.  `std::out_of_range(std::string(...))` had no viable constructor and
the runtime exception classes only took a `const char *` and stored it
without owning it.  `<stdexcept>` now has the standard class set with both
message constructors and an owned copy.  Retained as
`features/Exceptions/pass/test_standard_exception_message_from_string.cpp`.

### boost::predef: the missing `ntverp.h`

Closed.  `boost/predef/platform/windows_uwp.h` includes `<ntverp.h>`
unconditionally on Windows and the vendored SDK does not ship it.
`src/include/runtime/ntverp.h` now reports the pre-UWP Windows 7 SDK build the
vendored headers match, so UWP detection stays off.  Retained as
`features/Includes/pass/test_include_ntverp.cpp`.

### boost::numeric::conversion: `std::float_round_style`

Closed.  `integral_c<std::float_round_style, std::round_toward_zero>` needs
the floating-point style enumerations of [limits.numeric], which `<limits>`
did not declare.  The enumerations and the `round_style` member were added.
Retained as `features/Includes/pass/test_limits_float_round_style.cpp`.

### boost::container's `ordered_range`: a constant class initializer

Closed.  `static const ordered_range_t ordered_range = ordered_range_t();` is a
constant initializer: the class has no user-provided constructor, so `T()` is a
constant expression, while `can_lower_global_dynamic_init` refuses to lower a
`const` class to a dynamic initializer.  The static initializer path now owns a
class-typed value: a value-initialized trivially constructible temporary is
recorded as zero bytes for its scalar subobjects during a constant evaluation,
and `init_putv` writes the recorded bytes as static data.  Retained as
`features/Cpp17Gaps/pass/test_const_class_functional_initializer.cpp`.

### Explorer++ probe: a replayed constructor body under a static initializer

Closed.  The probe log
`Compatibility\build\explorer-probe9.log` reported
`boost/container/container_fwd.hpp:46: error: constexpr increment requires an
evaluation-owned object`.  That header was only where the parser happened to
be: while folding a namespace-scope static initializer, resolving the
initializer's constructor call flushed the queued member bodies, and each
replayed body inherited the fold state, so the ordinary `++` in
`std::basic_string`'s iterator constructor was read as constant evaluation.
`compile_pending_member_funcs` now suspends the static-initializer fold and the
evaluation-owned temporary depth while it compiles a member body for emission.
The local reduction is
`Compatibility\build\pending-flush-probes\h6_unused_make_box.cpp` (27 lines);
retained as
`features/Cpp17Gaps/pass/test_static_initializer_template_constructor_replay.cpp`.

### The missing `<cfloat>` runtime header

Closed.  With the replayed-body floor closed, the consumer probe moved to
`boost/math/tools/config.hpp:21: error: include file 'cfloat' not found`
(`Compatibility\build\explorer-stdafx.candidate.log`).  The runtime shipped
`<climits>`, `<cmath>` and `<float.h>` but no `<cfloat>` wrapper.
`src/include/runtime/cfloat` now includes `<float.h>`; retained as
`features/Includes/pass/test_include_cfloat.cpp`.

### boost::mpl's `vector0<>` argument list

Closed.  The consumer probe stopped in
`boost/mpl/vector/aux_/vector0.hpp:45`
(`Compatibility\build\explorer-probe11.log`).  Inside `vector0<na>`,
unqualified `vector0` resolved to its injected specialization token, but the
template-argument parser only looked for a member template under that token
and returned before consuming the following `<`.  It now uses the owning
class-template lookup for an injected class-template name, so `vector0<>`
closes its empty argument list and instantiates `vector0<na>`.  Retained as
`features/Templates/pass/test_injected_class_template_empty_arguments.cpp`.
