# CPrime Racer Compatibility Task

Last updated: 2026-08-22

## Goal

Extend compiler compatibility for Racer until this file exists:

`C:\Luke\Src\OT\cl\builds\racer\Racer.exe`

Use this file to run the current build attempt:

`C:\Luke\Src\CPrime\build_racer.ps1`

Do not call Racer ready to test until `Racer.exe` exists at the path above.

## Build Runtime Guardrail

CPC compiles are expected to be near-instant for the focused Racer objects.
Never leave a `cpc.exe` compile running for minutes. Broad Racer builds and
focused object compiles must be run with explicit, short timeouts while
compatibility work is still in progress. If an object compile exceeds the
expected quick compile window, stop that compile, treat it as a compiler hang
or pathological slowdown, reduce it, and fix the compiler path before running
another broad build. Do not wait on a long-running `cpc.exe` process in the
background, and do not start overlapping Racer builds.

## Current State

- `C:\Luke\Src\CPrime\cpc.exe` rebuilds successfully.
- `C:\Luke\Src\OT\cl\builds\racer\Racer.exe` is not produced yet.
- Verified compiler progress from earlier waves: fixed the original
  `additional_clFolder` access violation, added type-trait/template handling
  needed by `clDecryptThis`, fixed constructor conversion and overload ranking
  issues, and cleared isolated compiles for several additional Racer objects.
- Latest compiler progress: moved `additional_clRenderObjectCore` past the
  original chained `clString` operator+ failure, range-for declaration/body
  capture failures, functional enum cast failures, and generated `this`
  redeclaration failures.
- Latest compiler progress: added focused reducers for template auto-return
  overload arity, literal-plus-class chained operator+, braced forwarded member
  calls, range-for body capture, functional enum casts, and out-of-class member
  range-for field access.
- 2026-08-22 progress: fixed saved static-inline member codegen for inline
  member functions returning references by aligning generated inline receiver
  symbols with rewritten body tokens. Added passing coverage:
  `Tests\features\Classes\pass\test_inline_member_reference_return_lvalue.cpp`.
- 2026-08-22 progress: fixed first free operator overload registration when no
  mangled overload name is needed, so struct-return free operators are found
  before the same-struct fallback. Added passing coverage:
  `Tests\features\OperatorOverloads\pass\test_free_operator_struct_return_member_call.cpp`.
- 2026-08-22 guardrail: `build_racer.ps1` now enforces a per-object CPC
  compile timeout (`-CompileTimeoutSeconds`, default 10). A timed-out compile
  is killed and reported as `exit=-999 TIMEOUT`; do not run unbounded broad
  Racer builds.
- 2026-08-22 progress: fixed the `additional_clCamera` inline member-template
  auto-return `operator*` path enough to move past the old implicit `XYZ`
  diagnostic. Added passing coverage:
  `Tests\features\Templates\pass\test_inline_member_template_auto_operator_vector_factory_member_call.cpp`.
- Current `additional_clImage` finding: the active blocker is a compiler
  timeout/hang, not the older `clAABB3` redefinition note below. Bounded probes
  show prefixes through `clImage.cpp:169` finish, while completing
  `Resize(const i64&, ...)` at line 180 causes a runaway after the bogus
  diagnostic `error: ';' expected (got 'y')`. The latest reduced trace points
  at malformed constructor-initializer/template replay around a leaked
  `y(...) {}`-style token sequence after `clVector2` construction. Earlier
  diagnostics in that path include incorrect `assignment of read-only
  location` warnings on local bool initializers in the const
  `Resize(const clVec2I&, ...)` method.
- `additional_clFolder`, `additional_clDecryptThis`, `additional_cl2DDraw`,
  `additional_clScan`, and `additional_clPath` have each compiled in isolated
  checks with the Racer flags.
- Current handoff point: fix `additional_clImage` first. Keep every repro and
  object compile under a hard timeout. Do not keep temporary reducers under
  `build/`; promote only the minimal fixed reproducer into
  `Tests/features/.../pass`.
- Current `build_racer.ps1` result:
  - additional source files still have compile failures.
  - link has not been rerun to completion in this wave.

## Verified Tests

- `Tests\features\Templates\pass\test_template_base_qualified_member_call_uses_this.cpp`
- `Tests\features\Classes\pass\test_range_for_nested_class_reference.cpp`
- `Tests\features\Templates\pass\test_variadic_function_template_recursive_min.cpp`
- `Tests\features\Templates\pass\test_template_typedef_specialization_field_complete.cpp`
- `Tests\features\Templates\pass\test_template_forward_decl_after_definition.cpp`
- `Tests\features\Constructors\pass\test_functional_constructor_nested_operator_arg_replay.cpp`
- `Tests\features\Constructors\pass\test_return_constructs_via_constructor_argument_conversion.cpp`
- `Tests\features\Constructors\pass\test_member_initializer_reads_field_from_rvalue_ref_param.cpp`
- `Tests\features\Classes\pass\test_out_of_class_member_body_unqualified_nested_enum.cpp`
- `Tests\features\Classes\pass\test_inline_member_unqualified_static_const_default_and_body.cpp`
- `Tests\features\Classes\pass\test_out_of_class_operator_body_unqualified_member_call.cpp`
- `Tests\features\Classes\pass\test_wstring_return_value_cstr_shell_call.cpp`
- `Tests\features\Classes\pass\test_out_of_class_unqualified_overloaded_member_call_with_pointer_args.cpp`
- `Tests\features\Classes\pass\test_out_of_class_static_member_calls_static_member.cpp`
- `Tests\features\Templates\pass\test_standard_type_trait_values.cpp`
- `Tests\features\Classes\pass\test_compound_assignment_uses_converting_constructor_default_arg.cpp`
- `Tests\features\Classes\pass\test_static_overload_prefers_const_char_pointer_over_class_conversion.cpp`
- `Tests\features\Classes\pass\test_nonstatic_member_unqualified_call_prefers_static_overload_by_arity.cpp`
- `Tests\features\Constructors\pass\test_base_constructor_initializer_out_of_class.cpp`
- `Tests\features\Classes\pass\test_return_braced_initializer_list_constructs_class.cpp`
- `Tests\features\Classes\pass\test_conditional_with_auto_unary_member_result.cpp`
- `Tests\features\Classes\pass\test_out_of_class_inherited_unqualified_member_call.cpp`
- `Tests\features\Classes\pass\test_inherited_member_call_with_chained_auto_operator.cpp`
- `Tests\features\Templates\pass\test_free_template_auto_return_overload_by_vector_arity.cpp`
- `Tests\features\OperatorOverloads\pass\test_literal_plus_class_chained_operator.cpp`
- `Tests\features\OperatorOverloads\pass\test_literal_plus_class_free_template_operator.cpp`
- `Tests\features\Classes\pass\test_braced_list_forwarded_to_member_call.cpp`
- `Tests\features\Classes\pass\test_else_range_for_with_unbraced_if_body.cpp`
- `Tests\features\Classes\pass\test_range_for_variable_named_attribute.cpp`
- `Tests\features\Classes\pass\test_range_for_nested_type_in_out_of_class_method.cpp`
- `Tests\features\Classes\pass\test_functional_enum_cast.cpp`
- `Tests\features\Classes\pass\test_functional_enum_cast_call_arg.cpp`
- `Tests\features\Classes\pass\test_functional_enum_cast_member_call_arg.cpp`
- `Tests\features\Classes\pass\test_functional_enum_cast_local_in_method.cpp`
- `Tests\features\Classes\pass\test_out_of_class_member_range_for_field_unique_this.cpp`
- `Tests\features\Classes\pass\test_inline_member_reference_return_lvalue.cpp`
- `Tests\features\OperatorOverloads\pass\test_free_operator_struct_return_member_call.cpp`
- `Tests\features\Templates\pass\test_inline_member_template_auto_operator_vector_factory_member_call.cpp`

Focused checks from the latest wave passed for the reducers listed above.
The full surrounding suites still need to be rerun after the next coherent
fix for the current access violations.

## Current Compile Failures From `build_racer.ps1`

- Active first failure: `additional_clImage` times out after the malformed
  parser diagnostic `clImage.cpp:2: error: ';' expected (got 'y')`.
- The real bounded `additional_clImage` compile still exits as timeout
  (`exit=-999`) with the current rebuilt `cpc.exe`.
- Prefix probes:
  - `clImage.cpp` through line 169 completes under the timeout.
  - `clImage.cpp` through line 180 runs away after completing the second
    `Resize(const i64&, ...)` overload.
- Temporary instrumentation showed the bogus diagnostic near a replayed
  `y(...) {}` token neighborhood, which suggests malformed replay/consumption
  around `clVector2` constructor initializers or inline/template constructor
  bodies.
- Continue to `additional_clCamera`, `additional_clRenderObjectCore`, and
  `additional_clRenderObject` only after `additional_clImage` is stable.

## Current Link Undefined Symbols

```text
_ftelli64
_InterlockedExchangeAdd
alloca
cl2DDraw_constructor
cl2DDraw_Draw
cl2DDraw_Load
cl2DDraw_Load_struct_clImage_rref_struct_clVector2__float_float
cl2DDraw_SetAngle
cl2DDraw_SetPos
clCamera_LookAt
clContains
clDecryptThis_Crypt
clFolder_Exists
clFPSCamera_constructor
clImage_constructor_struct_clArray2__ui32_rref
clImage_constructor_uint_ptr_struct_clVector2__i32_ref
clPath_Path_const
clRenderObjectCore_destructor
clWindow_Aspect_const
clWindow_Clear
clWindow_destructor
clWindow_ScreenResolution
clWindow_Swap
strnlen_s
VertexType
```

## Remaining Work

1. Fix the current `additional_clImage` timeout/hang. Start from the reduced
   boundary: `clImage.cpp` prefix through line 169 finishes; completing
   `Resize(const i64&, ...)` at line 180 causes runaway after
   `';' expected (got 'y')`.
2. Keep every reproduction and focused Racer object compile under a hard
   timeout. Kill timed-out `cpc.exe` processes and treat them as compiler
   hangs/pathological slowdowns.
3. Add a passing regression for the fixed `clImage` compiler failure. Do not
   keep temporary reducers under `build/`; promote only the minimal fixed case
   into `Tests/features/.../pass`.
4. Rebuild `cpc.exe`.
5. Run focused tests:
   - existing inline auto-operator regression:
     `Tests\features\Templates\pass\test_inline_member_template_auto_operator_vector_factory_member_call.cpp`
   - new `clImage` timeout regression
   - nearby suites touched by the fix
6. Run bounded focused Racer object compiles only:
   - `additional_clImage`
   - `additional_clCamera`
   - `additional_clRenderObjectCore`
   - `additional_clRenderObject`
7. Continue fixing the next object failure, one bounded object at a time.
8. Only after focused objects are stable, run:
   `.\build_racer.ps1 -SkipLink -CompileTimeoutSeconds 10`
9. Then run the full link and resolve remaining undefined symbols or compiler
   failures.
10. Verify that this exact file exists:

`C:\Luke\Src\OT\cl\builds\racer\Racer.exe`

Do not report Racer as ready to test until that file exists. Report any
runtime-affecting warning classes that remain, especially implicit
declarations, pointer/integer casts, incompatible pointer assignments, and
read-only assignment warnings.
