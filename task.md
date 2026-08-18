# CPrime C++ Compatibility Push

Last updated: 2026-08-16

## Goal

Move CPC toward practical full C++ support by compiling the CommonLibrary/Racer
measurement project, reducing each real failure to a focused test, fixing CPC,
and rerunning the project.

## Workflow

1. Confirm the project symptom is real.
2. Add the narrowest repro test.
3. Fix only after the repro fails.
4. Verify focused tests.
5. Re-run Racer and record the next blocker.

## Completion and Handoff Standard

This task requires a sustained compatibility push. Moving Racer from one
diagnostic to another is intermediate evidence, not a completed work unit and
not a reason to stop. Continue across related blockers while useful local work
is possible, including reduction, implementation, compiler rebuilds, focused
tests, surrounding regression suites, and repeated full Racer builds.

Retained work must form a coherent forward-progress wave:

- no knowingly failing active-task reproducer filed under `fail` as a substitute
  for implementing the required behavior;
- no speculative or project-name-specific compiler hacks retained after they
  fail verification;
- failed approaches are removed before handoff;
- implementation and passing regression coverage are kept together;
- CPC is rebuilt and the real Racer compile/link is rerun after each meaningful
  fix wave;
- changes should accumulate into substantial, reviewable commit-sized units,
  rather than tiny diagnostic checkpoints.

The user-facing handoff condition is a Racer executable compiled with the newly
built CPC and verified to exist at a stated path. Do not announce that the user
can test until that condition is met. If a true blocker requires user input,
identify the exact missing input and the exhausted in-scope alternatives; an
ordinary compiler failure is not such a blocker.

## Measurement

Main project command:

```bat
cd /d C:\Luke\Src\OT\cl
build_prime.cmd
```

The build script uses `C:\Luke\Src\OT\cl\cpc.exe`, so after rebuilding CPC:

```bat
copy /y C:\Luke\Src\CPrime\cpc.exe C:\Luke\Src\OT\cl\cpc.exe
```

Useful direct probe pattern:

```powershell
$args = @('@C:\Luke\Src\OT\cl\builds\core\generated_core_cpc_flags.rsp',
          '-c','<source.cpp>','-o','C:\Luke\Src\OT\cl\builds\core\probe.obj')
& C:\Luke\Src\CPrime\cpc.exe @args *> C:\Luke\Src\OT\cl\builds\core\probe.log
```

PowerShell treats `@...` specially, so use an args array or `cmd /c`.

## Current Status

Done recently:

- Multi-parameter class/function templates.
- Function-template overload registration and declaration/definition merging.
- Explicit class/function template specializations.
- Namespace-qualified template/static calls used by runtime `<limits>`/`<cmath>`.
- `clList<T>` reference return declarations and const overload replay.
- Class-scope `using` aliases inside templates.
- Macro-expanded helper/static-member template replay used by `clReal`.
- PolyModel access violation from freeing pending template-spec scratch buffers.
- Array-reference constructor parameters.
- Explicit destructor calls/definitions with template-id spellings.
- Out-of-class member-template constructor definitions for concrete non-type args.
- Unused out-of-class variadic member templates are deferred.
- Self template-id member parameter/return declarations.
- Rvalue-reference type identity is preserved through member replay/mangling.
- Class-scope `using` aliases are renamed per class-template specialization.

Focused tests from the latest wave pass:

```bat
.\cpc.exe -run Tests\features\Constructors\pass\test_array_reference_constructor_parameter.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_out_of_class_member_template_definition.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_explicit_destructor_template_id.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_out_of_class_method_explicit_destructor_template_id.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_unused_out_of_class_variadic_member_template.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_member_rvalue_and_const_reference_overloads.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_class_using_alias_multiple_specializations.cpp
```

## Active Blocker

Racer moved past the previous:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Containers/clList.h:8: error: '(' expected (got '<')
```

Racer also moved past:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Containers/clList.h:30: error: redeclaration of 'this'
```

That trigger was:

```cpp
template <typename U> explicit clList(const clList<U> other)
  : m_size(0), m_capacity(0), m_pData(nullptr)
{
  Reserve(other.Size());
  for (const U &o : other) PushBack(T(o));
}
```

Notes:

- A close synthetic inline member-template constructor repro passes:
  `Tests\features\Templates\pass\test_template_inline_converting_constructor_template_parameter.cpp`.
- The immediate fix was to avoid emitting a generated replay prototype for
  lifecycle member-template specializations whose signature contains
  `std::initializer_list<T>`, because parsing that prototype can instantiate
  nested template lifecycle functions and collide on the synthetic `this`
  parameter.

Racer also moved past:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Containers/clList.h:121: error: incompatible types for redefinition of 'clList__ui8_PushFront'
```

Trigger in `clList`:

```cpp
void PushFront(T &&value);
void PushFront(const T &value);
```

Racer also moved past:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Strings/clToString.h:15: error: incompatible redefinition of 'iterator'
```

Fix notes:

- `T&&` now keeps a distinct `VT_RVALUE_REFERENCE` bit, emits `&&`, and mangles
  as `_rref` instead of collapsing with `T&`.
- Replayed out-of-class class-template member definitions resolve against the
  instantiated parameter signature before choosing a generated member symbol.
- `using` aliases in class-template specializations are rewritten like typedef
  aliases, e.g. `clList__ui8__iterator`, so different specializations do not
  collide on `iterator`.

Current blocker moved past:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Strings/clString.h:69:
error: too few arguments to function
```

Fix notes:

- Ordinary free/static function calls now emit saved default arguments for
  omitted trailing parameters.
- Function definitions inherit default-argument token strings from matching
  earlier declarations before installing the definition type.
- Overload metadata records accepted argument ranges and canonical function
  types so static member calls like `Class::Function(arg)` can use defaults.

Focused tests added/passing:

```bat
.\cpc.exe -run Tests\features\Classes\pass\test_free_function_default_arg_from_prototype.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_free_function_default_arg_preserved_after_definition.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_static_member_default_arg_preserved_after_definition.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_member_overloads_with_default_args_decl_only.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_member_overload_returns_self_by_value.cpp
```

New current blocker:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Strings/clString.h:123:
error: struct or union expected
```

Reduction notes:

- A direct include-only probe for `clString.h` with Racer flags reproduces the
  same failure, so this is header/inline replay, not a later Racer body issue.
- Preprocessed `clString.h` shows the likely trigger is an inline member body
  using an unqualified overloaded member call such as:

```cpp
*this = Substring(index + target.Length());
```

- Expected-fail coverage records the limitation:
  `Tests\features\Classes\fail\test_inline_member_unqualified_overload_call_returns_self.cpp`.

Reduction notes:

- Full generated all-source compile-only now exits 0:

```bat
cd /d C:\Luke\Src\OT\cl\Projects\Model Viewer
C:\Luke\Src\OT\cl\cpc.exe @C:\Luke\Src\OT\cl\builds\core\generated_core_cpc_flags.rsp -c @C:\Luke\Src\OT\cl\builds\core\generated_core_sources.rsp
```

- The standalone `Racer.cpp` access violation was reduced to an empty same-dir
  probe containing only:

```cpp
#include "clControls.h"
#include "Racer.h"

void Racer() {}
```

- The access violation is fixed; that reduced probe now exits normally with a
  diagnostic.
- The static `std::is_trivially_copyable<T>::value` path is fixed.
- `Reserve(o.Size())` in `clList<T>` copy construction is fixed by resolving
  unqualified out-of-class member calls through `this`.
- `clMemcpy(m_pData, o.m_pData, o.Size() * sizeof(T))` moved after allowing
  temporary expressions to bind to `const T&` parameters.
- `PushBack(o)` moved after preserving lvalue/rvalue category during overload
  probing, so lvalues no longer match `T&&` overloads.
- `: clList()` delegating constructors without an explicit template-id moved
  after treating unknown class-template initializer names as delegating
  constructors instead of fields.
- The unqualified member-template call `Resize(count, value)` is now recognized
  during replay and lowered through `this->Resize(...)`; the reduced probe no
  longer warns about an implicit free `Resize`.
- Variadic member-template bodies are still not instantiated, so recognized
  member-template calls are emitted as lowered external member-template targets.
- The remaining reduced diagnostic is now only `too few arguments to function`
  at `clString.h:69`, after the `Resize` warning is gone.
- Focused tests now cover the adjacent fixes for non-owning template replay,
  defaulted member declarations, friend declarations, parameter array
  incomplete types, unnamed defaulted template parameters, class-scope template
  skipping, static const data member initializers, static template `::value`,
  out-of-class member calls, const-reference temporary binding, lvalue/rvalue
  overload selection, and template delegating constructors without `<T>`.

Next wave:

- Reduce the remaining `clString.h:69` / `too few arguments to function`
  diagnostic now that `Resize` is recognized.
- Decide whether the remaining failure is from default arguments, defaulted
  assignment declarations, or another member overload/call replay edge.
- Re-run the empty same-dir `clControls.h` probe, then `Racer.cpp -c`, then the
  full generated all-source compile/link commands.

## Remaining Known Follow-Ups

- General variadic pack expansion remains incomplete. Existing fail repros cover
  richer pack forwarding and template calls after variadics.
- Pointer/reference overload ranking still has expected-fail coverage.
- Old include-order probes showed `cpcMesh.h` before `windows.h` can silently
  exit; recheck after current `clList` blocker.
- Full executable build still needs a final link/exe check after compile
  blockers are cleared.

## 2026-08-17 Wave Notes

Fixed and verified:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_function_template_infers_char_from_pointer.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_function_template_infers_class_from_pointer.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_function_template_infers_long_long_from_pointer.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_macro_default_type_parameter_skipped.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_member_overload_prefers_exact_arity_over_default.cpp
```

Notes:

- Function-template argument inference now maps pointer-to-`char`,
  pointer-to-class, and pointer-to-`long long` argument shapes.
- Function-template parameter scanning now skips balanced non-type/default
  parameter expressions so macro-style defaults do not split on inner commas.
- Member overload resolution now prefers exact arity over a candidate that only
  matches by consuming trailing default arguments.
- `<utility>` now provides `std::declval` in both staged runtime headers; the
  local AppData CPC include copy was updated for the Racer probe environment.

Current direct `clString.h` include probe with Racer flags still fails before
Racer can be tried:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Containers/clList.h:26:
error: '_len' undeclared
```

The reduced path is eager replay of a `clList` member-template constructor/body
that contains array/range handling before its non-type template parameter is
known. A broad defer attempt fixed this probe path but regressed
`test_template_out_of_class_member_template_definition.cpp`, so it was reverted.
Next work should add lazy member-template constructor replay for actual calls,
then defer eager replay of member-template lifecycle bodies safely.

## 2026-08-17 Later Wave Notes

Fixed and verified in focused tests:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_initializer_list_enum_type.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_initializer_list_enum_braced_temporary.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_initializer_list_enum_member_signature.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_initializer_list_enum_constructor_decl.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_initializer_list_enum_repeated_materialization.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_initializer_list_typedef_char_repeated_materialization.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_initializer_list_float_repeated_materialization.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_deleted_default_constructor_declaration.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_functional_constructor_two_arguments.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_typedef_functional_constructor_two_arguments.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_class_repeated_member_param_after_typedef_instantiation.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_member_template_binary_operator_same_type_fallback.cpp
```

Notes:

- `std::initializer_list<T>` replay now preserves enum and signed-char identity
  well enough to avoid repeated `begin()` materialization conflicts.
- Namespace-qualified nested class-template types are materialized while
  generating class specializations, which avoids delayed `initializer_list<T>`
  failures in constructor signatures.
- Deleted lifecycle declarations such as `Ctor() = delete;` are accepted.
- Eager class-template member replay no longer compiles default constructors
  just because a specialization appears in a reference parameter declaration.
- Pending template member bodies whose owning specialization is still
  incomplete stay queued instead of compiling with missing fields.
- Typedefs to class types now route functional construction like `v2I(96, 48)`
  through constructor parsing instead of scalar functional-cast parsing.

Current direct `Racer.cpp` compile moved through the headers and now reaches the
function body:

```text
C:/Luke/Src/OT/cl/Projects/Model Viewer/src/Private/Racer.cpp:17:
error: cannot convert 'struct clVector2__i32' to 'int'
```

The current reduced shape is `auto value = lhs - rhs;` where `lhs` and `rhs`
are same-type class-template instances and the only declared operator is a
skipped member-template `operator-`. CPC now carries the class type through the
operator fallback, but local `auto` declarations still default to `int`, causing
the conversion failure. Next work: implement local `auto` initializer type
inference for struct/class expressions, then rerun direct `Racer.cpp -c`.

## 2026-08-18 Wave Notes

- Local `auto` initializer inference is present in the current compiler and the
  former line-17 Racer failure no longer reproduces.
- Direct `Racer.cpp` compilation next failed at line 63 because the chained
  `auto` return path `clRotateVector(...) -> clCreateVector(...)` was typed as
  scalar `float` instead of `clVector2<float>`.
- A focused expected-fail reduction was added:
  `Tests/features/Templates/fail/test_auto_return_template_chained_class_factory.cpp`.
- Return scanning now recognizes a class-template construction in an `auto`
  function-template body and follows a simple nested function-template return
  chain. This moves Racer beyond line 63.
- The next direct/full-build blocker is currently reported at `Racer.cpp:34`:

```text
error: ';' expected (got 'v2')
```

- Verification also exposed the related underlying limitation in the focused
  reduction: when CPC emits the instantiated wrapper body, its named
  class-template-specialization parameter is not available in the body
  (`'value' undeclared`). Racer is not ready for runtime testing yet.

## 2026-08-18 Sustained Implementation Wave

Implemented and verified during the current wave:

- Function parameter duplicate detection is now limited to the current
  parameter list. Re-entrant template parsing can no longer cause an unrelated
  retained parameter symbol to rename a real definition parameter to an
  anonymous `L.*` symbol.
- Class-template replay searches specifically for typedef symbols instead of
  accepting the first same-name raw-stack symbol. This moves the real
  `clAABB2<float>` replay through the shadowed `v2` alias.
- Auto-return body scanning only treats `return Class<T>(...)` as class-valued;
  static helper calls such as `return Helper<T>::Value()` no longer get
  misclassified as returning the helper class.
- Pending template compilation preserves the outer function/code-generation
  identity needed by nested instantiation. This moves
  `clDegreesToRadians<float>` through its nested
  `clRadiansPerDegree<float>` instantiation.
- Inferred template calls now reuse the instantiated function symbol's real
  parameter list, replacing only the return type when stronger class-valued
  inference is available.
- The chained auto-return reproducer is now passing coverage at:
  `Tests/features/Templates/pass/test_auto_return_template_chained_class_factory.cpp`.

Focused verification currently passing:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_auto_return_template_chained_class_factory.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_macro_template_constant_helper_then_function.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_member_template_binary_operator_same_type_fallback.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_free_function_default_arg_preserved_after_definition.cpp
```

The direct Racer compile now reaches the late body/template emission path. The
current first diagnostic is:

```text
Racer.cpp:120: error: cannot convert 'struct clVector2__int' to
'struct clVector2__float'
```

The remaining work in this wave is to reduce and fix that cross-element vector
operator/result inference, complete direct compilation, then run the full
generated link and regression suites. No Racer executable has been handed off.

## 2026-08-18 Link and Runtime-Source Wave

Implemented and verified:

- Namespace-scope class copy initialization is deferred into `.init_array` and
  placement-constructs the global object directly. Constructor side effects and
  resulting object state are covered by
  `test_global_static_class_copy_initialization.cpp`.
- A class functional construction now takes precedence over function-template
  metadata recorded under the same class token for templated constructors.
- Range-for lowering accepts named class references such as
  `for (GamePad &gamePad : controllers)` in addition to `auto &` and
  `const T &`.
- Braced class returns such as `return {x, y};` are lowered through the named
  return type's functional construction.
- The real legacy `src/Platform/clAssets.cpp`, added to the core measurement
  inputs as a direct probe, now compiles through its global `clString` objects
  and two-argument `clString(...)` return construction.

Focused tests passing:

```bat
.\cpc.exe -run Tests\features\Classes\pass\test_global_static_class_copy_initialization.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_functional_constructor_prefers_class_over_constructor_template.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_range_for_typed_class_reference.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_return_braced_class_value.cpp
```

Current runtime-source probes:

- `src/UI/clControls.cpp` moves through typed range-for and braced return
  parsing, then fails during deferred `clAbs<double>` emission at the non-type
  argument `std::is_signed<T>::value`.
- `src/Polygon/cl2DDraw.cpp` still fails in the first constructor initializer
  `m_pos(clVec2(0, 0))`; the reduction showed that nested functional temporary
  member initialization/copy elision is not yet correct. Failed lowering
  experiments and their failing probes were removed.
- `src/Raster/clWindow.cpp` currently requires the unavailable staged Windows
  SDK header `shellscalingapi.h` before its compiler behavior can be measured.

The generated core-only source list still omits these legacy implementations,
and the full link therefore remains incomplete. No Racer executable exists.

### Later runtime/link progress

- Added runtime implementations for the Windows-compatible `__movsb` and
  `__stosb` byte intrinsics used by `clMemory.h`.
- Added the existing architecture-specific `chkstk.S` object to both Windows
  runtime archive build paths, resolving large generated stack frames.
- Added direct runtime coverage for byte move/set intrinsics and a 12 KiB local
  stack frame.
- `static_assert` is now parsed and evaluated instead of emitted as an external
  function call. Assertions in unused eagerly replayed template bodies are
  consumed without failing until a real instantiation is available.
- Built-in `std::is_floating_point`, `std::is_signed`, `std::is_integral`, and
  `std::is_same` value queries now use their template argument types instead of
  always evaluating to false. This moves Racer through its floating-point
  template assertion.

Passing focused coverage:

```bat
.\cpc.exe -run Tests\features\All\pass\test_runtime_windows_byte_intrinsics.cpp
.\cpc.exe -run Tests\features\All\pass\test_runtime_large_stack_probe.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_static_assert_statement.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_unused_template_static_assert_false.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_standard_type_trait_values.cpp
```

The real full link no longer reports `__movsb`, `__stosb`, `__chkstk`, or
`static_assert`. It still lacks the omitted legacy runtime implementations and
several template/lifecycle definitions, so there is still no Racer executable.

### Boolean specialization and legacy source progress

- Substituted template arguments now fold the supported standard type traits
  through `::value`, including nested use such as
  `Helper<T, std::is_signed<T>::value>`.
- Simple boolean non-type partial class specializations are selected and
  replayed with the complete specialization argument list. Runtime coverage
  verifies that signed and unsigned instantiations select different bodies.
- Braced class returns accept the standard trailing comma after the final
  initializer, including conditional expressions in the initializer list.
- With these changes, the real `src/UI/clControls.cpp` and
  `src/Platform/clAssets.cpp` probes both compile successfully. The measurement
  source also contained a genuine `clList<T>::PushBack` typo (`elements`
  instead of its declared `values` parameter), which was corrected in the
  CommonLibrary working tree.

The full core-only link still omits the legacy translation units and reports
their APIs as undefined. Direct `src/Polygon/cl2DDraw.cpp` compilation remains
blocked by constructor member-initializer overload lowering, first at
`m_pos(clVec2(0, 0))`. Incorrect compile-only lowering attempts were removed.
No Racer executable exists yet.

### Constructor initializer replay wave

- Constructor initializer lists encountered before later data-member
  declarations are corrected during deferred member-body replay, when the
  completed class layout and member types are available. Class-valued entries
  lower to placement construction rather than an invalid scalar assignment.
- Placement and functional default construction support aggregate classes
  without a user-declared constructor and initialize class-valued members.
- Runtime regressions cover direct multi-argument member construction, nested
  functional temporary/copy construction, overloaded placement construction,
  and default aggregate functional construction.
- All 28 constructor feature tests pass.

The real `src/Polygon/cl2DDraw.cpp` probe now moves through all three
constructors and the former `m_model = clRenderObject()` aggregate construction.
Its current first error is initializer-list construction at line 118,
`clList<clVec3> posData = { ... }`, where overload selection attempts to
convert a `clVec3` element to the list's integer size constructor. No Racer
executable exists yet.

An attempted weak-linkage treatment for generated template lifecycle methods
was removed after it caused a reproducible vstack leak in the original Racer
core build. The verified core build is back to the unresolved-symbol link
stage; it does not retain that failed linkage experiment.

### Braced initializer-list construction wave

- Braced copy-list initialization of a class now prefers its one-argument
  `std::initializer_list<T>` constructor instead of treating each list element
  as an argument to an unrelated one-argument constructor.
- Materialized initializer-list temporaries now have backing element storage
  and a valid pointer/count layout rather than a null pointer paired with a
  nonzero count.
- All 29 constructor feature tests pass, including
  `test_braced_initializer_list_constructor_overload.cpp`.
- The real `cl2DDraw.cpp` probe moves beyond its line-118 `clList<clVec3>`
  diagnostic. Its next clean diagnostic is the deferred default argument
  `GL_REPEAT`, whose macro expansion context is not retained. An eager saved
  default-token expansion attempt moved both this probe and the original Racer
  core build into a late deferred-emission access violation, so that attempt
  and its temporary tracing were removed.
- The original core Racer command remains stable at the unresolved-symbol link
  stage. Adding legacy sources to the same compiler invocation still exposes
  duplicate generated vector constructors, so no Racer executable exists yet.

### Legacy object and default-argument progress

- All 22 generated core sources compile successfully as independent objects.
  An object-only link reproduces the unresolved-symbol list without the
  same-session duplicate-template definitions, providing a viable route for
  adding legacy implementation objects incrementally.
- CPC's runtime `GL/gl.h` now supplies the standard `GL_REPEAT` texture wrap
  constant (with direct runtime-header coverage), moving `cl2DDraw.cpp` beyond
  its deferred OpenGL default argument.
- Function-template signature analysis now excludes parameters with defaults
  from the minimum call arity. The focused runtime test
  `test_function_template_default_argument.cpp` passes, and the intended
  two-argument `clMoveConstruct` calls in CommonLibrary now match after its
  count parameter was corrected to default to one.
- Standalone `clControls.cpp` and `clAssets.cpp` objects compile. Standalone
  `cl2DDraw.cpp` now reaches line 137, `m_image.Size().x`, where inherited
  member lookup through the templated `clArray2<ui32>` base still loses the
  class-valued return and reports `lvalue expected`.
- Two eager/lazy templated-base instantiation approaches fixed a focused
  reduction but disturbed the active enclosing class replay in the real file,
  causing missing `cl2DDraw` fields. Both approaches and their experimental
  test were removed; the retained compiler is back to the clean line-137
  diagnostic and the stable core linker stage.

### Split-link and Windows runtime-source progress

- `#pragma comment` accepts adjacent string fragments, including the common
  `MACRO "library.lib"` spelling used by MSVC projects.
- Template argument inference now preserves `bool`, fixing the real
  `clUnused(vsync)` overload set with const-reference and variadic templates.
- Typedef class names support braced temporary construction in call arguments,
  moving the real `POINT{...}` Windows call.
- The bundled Windows/OpenGL compatibility headers now include the execution
  state, monitor scaling, and legacy OpenGL constants exercised by
  `clWindow.cpp`, with focused passing header tests.
- After correcting a one-underscore `declspec` typo and making three intended
  `clString` converting returns explicit in CommonLibrary, standalone
  `clWindow.cpp` compiles successfully to
  `C:\Luke\Src\OT\cl\builds\split\legacy_window.obj`.
- The split link now has standalone `clControls`, `clAssets`, and `clWindow`
  objects available. It exposes two remaining classes of work: `cl2DDraw.cpp`
  is still blocked by templated-base inherited lookup at line 137, and inline
  generated vector lifecycle definitions collide across independently
  compiled objects. A direct storage-class change to the predeclared lifecycle
  symbol regressed constructor redefinition checks and was removed; weak
  linkage remains deliberately absent.

### Local aliases, cl2DDraw objects, and ODR link progress

- Local `using Name = Type;` declarations are now accepted inside ordinary
  and deferred template function bodies. Typedef-based scalar direct
  initialization such as `Element zero(make<Element>());` is also parsed as
  initialization rather than as a function declarator. The focused
  `test_local_using_alias.cpp` regression passes.
- `cl2DDraw` no longer depends on the compiler resolving inherited members
  through an erased templated base. Narrow `clImage` accessors are implemented
  in a separate source, sampling is independently compiled, and the concrete
  2D matrix transform is isolated in a C helper. The main draw source and all
  three helper sources now produce objects successfully.
- CommonLibrary's matrix operators now construct their concrete result type
  directly in the affected generic bodies, avoiding an unresolved nested
  static template call during deferred replay.
- The object linker coalesces byte-identical same-section definitions and
  template-specialization lifecycle definitions when input objects lack
  COMDAT metadata. The existing multi-source suite passes, and the Racer split
  link now reports zero duplicate definitions instead of the prior lifecycle
  and header-constant collisions.
- The real SDL2 DLL is accepted as an import source, removing the SDL import
  failures. The remaining split link reports 113 CommonLibrary/template and
  platform implementation symbols. There is still no Racer executable.

### Runtime algorithms and out-of-class operator replay

- CPC's runtime now provides the pointer-range `sort`, `lower_bound`,
  `upper_bound`, `replace`, and two-argument `shuffle` algorithms exercised by
  CommonLibrary. The focused `test_runtime_algorithm.cpp` compiles and runs.
- Out-of-class template `operator[]` definitions are no longer mistaken for
  declarations containing an unrelated array declarator. Their bodies are
  emitted even when the in-class prototype already exists. Both the direct
  operator-index test and the const/non-const reference-overload test pass.
- The full CommonLibrary string implementation now compiles as a standalone
  object after separating its wide-string-only constructor and spelling
  several ambiguous conversions and member calls explicitly.
- Runtime OpenGL compatibility includes the mipmap filters, depth format, and
  1D texture target used by `clHardwareTexture.cpp`; that source now compiles
  as a standalone object. Adding it and the string object leaves the split
  Racer link with zero duplicate definitions and 156 unresolved symbols.
  No Racer executable exists yet.

### Scalar placement construction investigation

- Scalar placement new expressions such as `new(ptr) int(value)` are now
  lowered by evaluating the destination once, storing the initializer, and
  returning the placement pointer. The focused
  `test_scalar_placement_new.cpp` regression passes.
- General emission of ordinary out-of-class class-template member bodies was
  investigated against the real `clString.cpp`. It exposed incompatible
  overload candidates being instantiated eagerly and then reached variadic
  forwarding through `clConstruct`. The incomplete broad emission change was
  removed; the retained compiler remains stable and `clString.cpp` compiles.
- Two genuine CommonLibrary `clList` defects found while exercising those
  bodies were corrected: `values.m_size.Empty()` now calls `values.Empty()`,
  and initializer-list `PushFront` delegates directly to `Insert`.
- CPC, all 22 core measurement objects, and all retained legacy/helper objects
  were rebuilt. All five multi-source groups pass. The clean split link has
  zero duplicate definitions and 150 unresolved symbols; no Racer executable
  exists yet.

### Rvalue-reference calls and C string runtime

- Free function calls now materialize rvalue temporaries when binding an
  rvalue-reference parameter. Named `T&&` function-template parameters and
  variadic forwarding into scalar placement construction therefore work.
- The former expected-failure variadic tests
  `test_template_after_variadic_template_keeps_own_name.cpp` and
  `test_variadic_template_body_call_does_not_rename_template.cpp` now compile
  and run as passing coverage. Focused rvalue-reference and templated scalar
  placement tests also pass.
- Added the standard `<cstring>` wrapper backed by CPC's existing
  `__builtin_memcmp` and `__builtin_strlen`. Its runtime test passes, and the
  real CommonLibrary `clMemory.cpp` now compiles after replacing unsupported
  `static_cast` spellings with equivalent C-style casts.
- Adding `clMemory.cpp` resolves the two `clStrLen` references but pulls in
  `_clAlloc`; adding `clAlloc.cpp` then pulls in its `_malloca`/`alloca`
  implementation, so the clean link total remains 150 unresolved symbols.
  All 22 core objects and all retained helper objects rebuild successfully,
  with zero duplicate link definitions and no Racer executable yet.

### Initializer-list access and utility swap

- Calls to `std::initializer_list<T>::size()` are lowered directly to the
  runtime object's length field, matching the existing direct `begin()`
  lowering and avoiding external inline-template symbols. Rebuilding all core
  and helper objects removes six initializer-list `size` failures from the
  real link.
- Runtime `<utility>` now supplies `std::swap`, and `clBuffer.h` includes the
  header it uses. The focused integer swap test passes without warnings and
  the real `cl2DDraw` object no longer leaves `__cpc_ns_std_swap` undefined.
- The clean Racer split link is now at 143 unresolved symbols, down from 150,
  with zero duplicate definitions. There is still no Racer executable.
- The real `clScan.cpp` also compiles after replacing three unsupported scalar
  functional casts and qualifying an overloaded static call. Its object is not
  retained in the final link yet because it introduces additional unresolved
  `clList<float>`/`clList<i64>` specializations and raises the total.
