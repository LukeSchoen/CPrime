# Known CPC issues

Open defects, each with its reduced shape and the work it needs. Completed work
is code and retained cases; nothing here is a progress log.

Consumers named here are the user's own checkouts outside this repository. Their
paths are per machine and are deliberately not written down: run the described
probe from wherever that machine keeps the checkout.

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

Closed.  `<locale>` now has `locale::facet` with the reference-counted
`_Incref`/`_Decref` pair and `locale::id`, and `locale(const locale&, Facet*)`
registers the new facet next to the inherited ones.  Each locale owns a small
fixed registry that stores the facet's type name beside the pointer, so
`has_facet`/`use_facet` find the registered object while the name (not a
per-type static address, which this compiler does not merge across translation
units) is the key.  `basic_ios` holds a `locale`, and `basic_istream`,
`basic_ostream` and `basic_iostream` expose `getloc`/`imbue`.

Retained as `features/Includes/pass/test_locale_facet.cpp`; it derives a facet,
checks that an unrelated facet is absent, registers one with
`std::locale(base, new facet)`, round-trips `imbue`/`getloc`, copies the locale
and replaces a same-type facet.  The reduced probes
`Compatibility\build\locale-probes\p1_facet_base.cpp` and
`p2_facet_on_stream.cpp` both compile and run.  The fast tier and the
regression gate were re-run and the compiler republished.

### Constructing a facet with arguments

Closed.  The reported shape was `new T(args...)` rejecting a class whose only
constructor converts.  `new F(7)` on the facet-like class below, and every
variant of it (a class with no default constructor, a deleted default
constructor, a converting constructor reached through a conversion, an
in-class or out-of-line definition, or a class-template specialization), now
compiles and runs, so the default-constructor demand no longer reproduces.

```cpp
struct F : std::locale::facet {
  F() : std::locale::facet(0) {}
  explicit F(int v) : std::locale::facet(0), m(v) {}
  int m;
};
void f() { new F(7); }
```

The defect that did reproduce in that area was the qualified spelling: the
`::` after `new` was read as the qualifier of a namespace named `new`, so
`new ::T(7)` became a call of an invented `new::T` function and the
constructor arguments were never seen.  The `new` keyword is now recognised
before the qualified-name path in the primary parser.  Retained as
`features/Expressions/pass/test_new_global_scope_qualified_type.cpp`.

## boost::type_traits: `is_base_and_derived_impl::type`

Open, and the current consumer floor.  With the facet surface published the
Explorer++ probe (`Compatibility\build\explorer-probe15.log`) advances past
`gregorian_io.hpp` and stops inside
`boost/date_time/gregorian/greg_weekday.hpp:215` with a very long
`nested template type member '...is_base_and_derived_select...::type' must be a
typedef` (`boost/type_traits/is_base_and_derived.hpp:205` and following).  The
mangled name repeats `is_base_of_imp<std::exception, ...>` and
`is_base_and_derived_impl<...>` several times, so it is the dependent
`typedef typename ...::type` lookup through
`boost::detail::is_base_and_derived_select`, not the earlier derived-to-`void*`
ranking fix.

Reduced so far: `Compatibility\build\date-time-probes\svp2.cpp` includes
`boost/date_time/gregorian/gregorian.hpp` and names
`special_values_parser<date, char>` and `<date, wchar_t>`; the streaming
headers are not needed to reach the floor, but the surrounding include order
is, which is why a smaller first reduction landed elsewhere.

Work required:

- Reduce the `is_base_and_derived_select` chain to a local case.
- Make the replayed class-template member typedef resolve through the
  `typename ...::type` chain.
- Retain the case in `features/Templates` and re-run the consumer probe.

## clCRC.cpp: a leading `::` in a replayed member function template

Closed.  An out-of-class member function template whose declaration and
definition spell a parameter type with a leading `::` (`::G`) was replayed
without its declarator, so the redefinition reported incompatible types
(`... versus 'inline unsigned long long'`).

`token_can_start_parameter_declaration` classified `(::type name)` as a direct
initializer expression, because `:` cannot begin a parameter declaration for
it.  The classifier now looks past a leading `::`: when the qualified name
that follows can itself start a parameter declaration the declaration reading
is kept, and a plain object name still reads as an initializer.  That single
misclassification was the whole family: a free function template, an in-class
member template, a class-template member, and a plain function definition with
a `::`-spelled parameter all failed before and compile and run now.  Retained
as
`features/Templates/pass/test_leading_global_scope_parameter_in_member_template.cpp`.

## faceDetectCNN.cpp: SSE and AVX vector types

Closed.  CPrime's `immintrin.h` was a stub, so `__m256` was undeclared and
`faceDetectCNN.cpp`, which defines `_ENABLE_AVX2` and includes the umbrella
header, could not compile at all.

The runtime `<immintrin.h>` now defines `__m64`, `__m128`/`__m128d`/`__m128i`,
`__m256`/`__m256d`/`__m256i` and `__m512`/`__m512d`/`__m512i` as GNU vector
extensions, plus the SSE/AVX/AVX-512 float kernels the CNN kernels use
(load/store, setzero/set1/set/setr, add/sub/mul/div, max/min, bitwise forms,
comparisons, movemask, horizontal add, unpack/shuffle, the scalar `ss` forms,
`_mm512_reduce_add_ps`), the 128/256-bit integer core, and the reinterpreting
casts.  Comparison intrinsics return the all-ones mask: the compiler's element
comparison yields `-1.0f`, so the sign bit is broadcast over each lane.  Thin
`xmmintrin.h`/`emmintrin.h`/`pmmintrin.h`/`tmmintrin.h`/`smmintrin.h`/
`nmmintrin.h`/`avxintrin.h`/`avx2intrin.h` wrappers include the umbrella
header.  With the published runtime, `faceDetectCNN.cpp` compiles (its only
remaining diagnostic is an unrelated `std::stable_sort` declaration warning).
Retained as
`features/Intrinsics/pass/test_sse_avx_intrinsics.cpp`, which checks every
intrinsic added against scalar arithmetic.

## clProcessList.cpp: psapi.h

`psapi.h` is absent from `third-party/win32-sdk/include`; the installed Windows
SDK ships `um/Psapi.h`.

Work required:

- Choose the packaging shape for the missing SDK header and add it.
- Compile and run the uses the project makes of the process API.

## RTMPose / Kpose kernels (Kinect tree, read only)

Convolutions dominate inference at roughly 85% of the time, and a hand-written
SSE GEMM measured about 5x faster than the C loop in isolation.  Both blocking
defects are closed below: the atomic work counter returns its result, and the
inline-SSE kernels build, verify and stay the fast path (`KPOSE_GEMM=sse` is
175.8 ms against 326.3 ms for the scalar kernel on the verification model).
`KPOSE_THREADS=1` forces single-threaded execution for A/B measurements. The
external reproducer and notes are in the Kinect tree's `README.md`; fixtures and
models are regenerated with `python tools\vendor_rtmpose.py --all`.

### InterlockedIncrement ignores its result

Closed.  The vendored `winnt.h` spelled both interlocked increments as an
inline-asm template that inferred the new value from the condition flags of
the `lock addl`/`subl`.  The template also names the operation's address
operand directly, and the inline-asm register allocator can give a following
flag output the same register, so the caller read a clobbered value: the
counter advanced while the result never did.

`_InterlockedIncrement` and `_InterlockedDecrement` are now runtime-library
helpers (`src/runtime/windows/winintrin.S`) built on `lock xaddl`, exactly
like the existing `_InterlockedExchangeAdd`, and `cprimedefs.h` marks the
runtime as owning them so `winnt.h` does not emit its own inline bodies.
Retained as `features/Abi/pass/test_msvc_interlocked_counter.cpp`, which
compares the returned value with the value left in memory across increment,
decrement and a mixed sequence.

### Inline SSE asm corrupts surrounding float code

Not reproducible with the published compiler; the reported shape is now
retained as a passing case.  The reduction is
`features/GnuExtensions/pass/test_inline_sse_asm_with_scalar_tail.cpp`: the
vector loop runs `movups`/`mulps`/`addps` on `xmm0`-`xmm2` with a 4-float
stride, and the leftover columns run scalar float arithmetic afterwards.  It
produces the scalar reference for `N=24`, `N=25` and `N=28`, and the
`N=25`-only failure does not appear.

The three behaviours the report rested on were checked separately and all hold
now:

- `movups` accepts an unaligned operand: a load/store through pointers at
  offsets 1, 2 and 3 floats all complete, so the `N=25`/`N=24` split (a row
  stride of 100 bytes against 96 and 112) is not an alignment fault.
- Floats live across the asm: ten locals read after an asm block that loads
  `xmm0`-`xmm7` keep their values, at `-O0` and `-O2`.
- The consumer's own kernels: `src\main.c` in the Kinect tree builds with root
  `cpc.exe`, and `kpose.exe --smoke` (9 passed) and `--verify` report
  `max abs error 2.289e-05` with `KPOSE_GEMM=sse` in 175.8 ms against
  326.3 ms for the scalar kernel, i.e. the inline-asm path is both correct and
  still the fast one.

## The standard `std::atomic_*` typedefs crash the compiler

Closed.  The standard typedefs are now in `src/include/runtime/atomic`.  The
crash was a double free while parsing conversion operators:
`make_type_from_saved_type_tokens` consumes its token string, but three
callers released it again.  The pool corruption became fatal once the
`<atomic>` typedef set enlarged the token stream.  Retained as
`features/Atomics/pass/test_atomic_typedefs.cpp`.

## Explorer++ probe: `std::string(begin, end)` from two pointers

The probe is the Explorer++ checkout, a manifest build of 232 translation units
driven by `src\scripts\project.exe`; entry point
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
