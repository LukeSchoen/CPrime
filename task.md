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
