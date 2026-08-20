# CPrime C++ Compatibility Push

Last updated: 2026-08-20 (late evening)

## 2026-08-20 Continued Wave: Static Qualified Calls, Runtime limits,
and clPath Split Object

Implemented and verified:

- Out-of-class static member-function bodies now replay already-qualified
  static calls as the registered static-member token.  This fixes ordinary
  and cross-class forms like `Folder::Create` calling `Folder::Exists(path)`
  or `File::Delete(path)`, which previously fell through to constructor
  probing and failed with undeclared `Class_Class` symbols.
- Added passing regression coverage:

```bat
.\cpc.exe Tests\features\Classes\pass\test_out_of_class_static_member_calls_static_member.cpp -o <temp>\static_member_call_repro.exe
```

The Classes suite now includes the new coverage and reports 52 passed / 6
failed.

- CPC runtime `<limits>` now undefines pre-existing function-like `min` and
  `max` macros before declaring `std::numeric_limits`, so including `<limits>`
  after C/Windows compatibility headers no longer fails with "macro 'min'
  used with too few args".  The installed extracted runtime header under
  `%LOCALAPPDATA%\cpc\1.4\include\limits` was refreshed for the measurement
  run.
- Added passing header coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_runtime_limits_after_minmax_macros.cpp
```

- CommonLibrary `clPath.cpp` was made explicit in several places where CPC
  still cannot reliably apply user-defined conversions or member-template
  stream helpers: path copy/move operations use explicit `clString(...)`
  construction, `ResolveRelativePath` builds a local source path directly,
  and `clPath` stream serialization uses raw stream reads/writes matching
  the existing `clString` wire format.  `commonlib_clPath` now compiles.
- `rebuild_split_new.ps1` now attempts the next missing CommonLib implementation
  sources (`clFolder`, `clDecryptThis`, `clCamera`, `clImage`,
  `clRenderObjectCore`, `clRenderObject`) so their real compile blockers are
  visible instead of remaining absent from the link.
- `clDecryptThis.cpp` had its active range-for loops rewritten as indexed
  loops and `clVec3::Zero()` replaced with an explicit zero vector; it now
  moves past the range-for/static-member blockers but still fails later in
  `Cypher`.

Measurement:

- CPC rebuild succeeds and replaces `C:\Luke\Src\CPrime\cpc.exe`; the rebuild
  still emits the pre-existing `token_string_has_variadic_pack` declaration
  order warnings.
- Core compile: all 22 generated core objects compile.
- Retained CommonLib/helper compile: all previously retained objects compile,
  including new `commonlib_clPath`.
- Newly attempted CommonLib sources still failing:
  `commonlib_clFolder` at nested `clFolder::FileInfo` lowering,
  `commonlib_clDecryptThis` at a later `Cypher` list/type issue,
  `commonlib_clCamera` at an int-to-vector constructor/conversion,
  `commonlib_clImage` missing `vnImagine.h`,
  `commonlib_clRenderObjectCore` at a `clHashMap` template redefinition, and
  `commonlib_clRenderObject` at braced initialization.
- Split link remains without a Racer executable.  With only successfully
  compiled objects, the current undefined set is 20 symbols:
  `__movsb`, `clCamera_LookAt`, `clContains`, `clDecryptThis_Crypt`,
  `clFolder_Exists`, `clFPSCamera_constructor`,
  `clImage_constructor_struct_clArray2__ui32_rref`,
  `clImage_constructor_uint_ptr_struct_clVector2__i32_ref`,
  `clRenderObject_DrawQuads`,
  `clRenderObject_GetShaders_struct_clString_ptr_struct_clString_ptr`,
  `clRenderObject_SetAttribute`,
  `clRenderObject_SetShaders_struct_clString_ref_struct_clString_ref`,
  `clRenderObject_SetTexture_struct_clString_ref_uint_ref_bool_ref`,
  `clRenderObject_SetUniform`, `clRenderObjectCore_destructor`,
  `clRenderObjectCore_operator=_struct_clRenderObjectCore_ref`,
  `clWindowCreateFlags`, `DoesContain`, `GlobalMemoryStatusEx`, `VertexType`.

## 2026-08-20 Final Wave: Variadic Resize Replay, Out-of-Class Member Scope,
and Explicit-Specialization Static Calls

The two remaining compiler-side emission gaps are fixed, and the real
`clString.cpp` compiles cleanly with the new compiler for the first time.

Root causes and fixes:

- Out-of-class member definitions that call members or construct class
  temporaries lost their implicit `this` mid-body.  Function-type
  construction helpers (`make_lifecycle_func_type`,
  `make_lowered_member_func_type`, `make_func_type_from_saved_params`) pushed
  their `this`-named parameter symbols onto the live local scope during
  expression parsing; the next enclosing scope pop unlinked the "this" token
  table entry and later out-of-class members (e.g. `clString::Substring`)
  failed with `'m_data' undeclared`.  Those helpers now push type symbols on
  the global stack (as they do at file scope), so scope pops cannot corrupt
  the token table.
- Variadic member-template bodies with an empty pack (e.g.
  `clList<char>::Resize(n)` with `Args&&... args` empty) failed to replay:
  the pack parameter stayed in the replayed signature and the pack-expansion
  expressions (`T(args...)`, `clForward(Args, args)...`) misparsed.  The call
  site now marks an empty pack explicitly; the replay drops the pack
  parameter from the signature and removes the expansion expressions,
  including postfix-expression prefixes such as `std::forward<char>`.
- The variadic instantiation cache was keyed only on the class type, so a
  zero-element and one-element pack instantiation collided.  The cache now
  also keys on the pack argument token.
- Variadic member instantiation was skipped when no regular overload was
  registered and the explicit argument count did not equal the parameter
  count.  Variadic members now bypass that arity guard so a zero-element
  pack (`Resize(n)`) instantiates.
- `identity<int>::value()` (and the real
  `clAdditiveIdentity<int>::Value()` / `clMultiplicativeIdentity<int>::Value()`)
  failed with "function pointer expected": the generic
  `Class<Args>::value` parse shortcut treated any `::value` as a standard
  type trait.  The shortcut now only fires for a non-call `::value` (static
  data member / trait constant); a `::value(` function call rewinds and
  resolves through the qualified static-member path.
- Explicit specializations registered with a typedef spelling of the
  argument (e.g. `clAdditiveIdentity<i32>` via the macro, called as
  `clAdditiveIdentity<int>`) created a duplicate primary-template instance
  whose static member was never defined.  Template instance lookup now
  compares arguments by underlying type, and the class instantiation path
  materializes a matching pending explicit specialization before creating a
  new instance.

New passing regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_variadic_member_resize_single_pack.cpp
```

The test uses the real `clList.inl` body with the `clScan.h`/`clSort.h`/
`clStream.h` include set and covers both zero-element (`Resize(10)`) and
single-element (`Resize(10, 'x')`) pack calls.

Suite comparison against the pre-wave compiler: Templates 154/16 -> 159/12
(four explicit-specialization static-member tests and the new resize test
fixed; zero regressions).  Constructors 31/0, Classes 51/6,
MemberFunctions 35/2, Destructors 14/0, OperatorOverloads 16/9, All 18/2,
c_compat 17/0 and MultiSource all unchanged.

Measurement: all 22 core objects and all retained CommonLib objects compile
with the new compiler, including the real `clString.cpp` (previously blocked
at `clString.cpp:107` 'm_data' undeclared and `clString.cpp:337` in the
Resize replay).  The clean split link dropped from 31 to 28 undefined
symbols: `clList__char_Resize`, `clAdditiveIdentity__int_Value` and
`clMultiplicativeIdentity__int_Value` are resolved.  No Racer executable
exists yet; the remaining set is dominated by CommonLib sources that still do
not compile (`clImage.cpp`, `clCamera.cpp`, `clRenderObjectCore.cpp`,
`clRenderObject.cpp`, `clHardwareTexture.cpp`, `clFolder.cpp`,
`clDecryptThis.cpp`, `clStringEncoding.cpp`, stream sources) plus the
runtime/OS imports (`alloca`, `_clRelFail`, `GlobalMemoryStatusEx`,
`SetProcessDpiAwareness`, `ImmAssociateContext`, `GetScaleFactorForMonitor`)
and missing definitions (`clWindowCreateFlags`, `VertexType`, `clContains`).

## 2026-08-20 Late Follow-up: Out-of-Class Lifecycle Emission, Nested Member
Calls, Defaulted Assignment, and Variadic Member Signatures

Wave on the remaining split-link undefined set:

- Out-of-class class-template lifecycle/operator bodies are now emitted for
  used specializations: default constructors reached through member default
  construction, `operator=` bodies (the blanket duplicate-members skip for
  `operator=` is removed), and variadic member templates whose body has no
  pack expansion (simple `Resize` shapes).  `resolve_lifecycle_func` now asks
  `instantiate_template_member_for_call` for constructor/destructor members,
  and lifecycle instantiation is allowed from inside non-lifecycle template
  member bodies so matrix/vector constructors referenced from replayed
  members are emitted too.
- Auto-return out-of-class member chains (`clVector2<T>::Length` ->
  `clVector2<T>::LengthSquared` -> `clSqrt(...)`) now infer scalar return
  types recursively and cache them per specialization, instead of leaving the
  queued body on `TOK_AUTO` forever.  A free-function wrapper around a member
  call (`clSqrt(LengthSquared())`) is lowered to the member's scalar type for
  the probe.
- Fixed a general codegen bug where a member-function call used as an
  argument to another function corrupted the value stack
  (`drop_leaked_call_target` now only pops when the top value is the actual
  leaked call-target symbol).  This is required by `clSqrt(LengthSquared())`
  and by ordinary `foo(obj.Method())` expressions.
- `= default` copy assignment on ordinary classes is now emitted per TU as a
  real global function (with a canonical unsuffixed symbol plus the mangled
  overload symbol), so `clString::operator=` no longer remains undefined at
  link.  Defaulted assignment with implicit conversions (`clAssetsPath =
  "Assets/"`) works through the normal overload path.
- Template type arguments that are typedefs to basic types (`i32`, `f32`)
  are canonicalized during argument parsing, and static member functions
  register their unsuffixed base symbol so qualified calls can resolve before
  argument parsing.

New passing regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_out_of_class_auto_member_call_chain.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_out_of_class_auto_member_free_func_wrapper.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_out_of_class_template_default_ctor_member.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_out_of_class_template_operator_assign.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_out_of_class_variadic_member_template.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_out_of_class_many_param_constructor.cpp
.\cpc.exe -run Tests\features\Classes\pass\test_defaulted_copy_assignment_emitted.cpp
.\cpc.exe -run Tests\features\MemberFunctions\pass\test_member_call_as_function_argument.cpp
Tests\test_MultiSource.cmd -CompilerPath cpc.exe
```

Suite comparison against the pre-wave compiler: Templates 148/20 -> 154/16,
Constructors 31/0, Destructors 14/0, MemberFunctions 35/2, Classes 51/6,
OperatorOverloads 15/10, MultiSource all groups plus the new defaulted
assignment group.

Measurement: the clean split link dropped from 37 to 32 undefined symbols.
Resolved in this wave: `clList__glResource_constructor`,
`clImage_constructor...` is still CommonLib-side, but the compiler-side lifecycle set
is mostly gone (`clList`/`clString` constructors and assignment, `clVector2`
Length, matrix/vector constructors, `clList<char>::Resize` simple shapes).
Remaining undefined symbols are dominated by CommonLib sources that still do not
compile (`clImage.cpp`, `clCamera.cpp`, `clRenderObjectCore.cpp`,
`clRenderObject.cpp`, `clHardwareTexture.cpp`, `clFolder.cpp`,
`clDecryptThis.cpp`, `clStringEncoding.cpp`, stream sources), the
still-skipped variadic `clList<char>::Resize` body with pack expansion, and
runtime/OS imports (`alloca`, `_clRelFail`, `GlobalMemoryStatusEx`,
`SetProcessDpiAwareness`, `ImmAssociateContext`,
`GetScaleFactorForMonitor`, `clWindowCreateFlags`, `VertexType`, `clContains`,
`clColor_UnpackVec3`).  No Racer executable exists yet.

## 2026-08-20 Evening Follow-up: Typedef-Token Identity and Deep Template Chains

Follow-up wave on the split-link undefined set:

- `std::remove_const<T>::type` / `std::remove_reference<T>::type` now return
  the canonical spelling of the type, so a typedef argument (`f32`) matches
  the plain-type explicit specialization (`clAdditiveIdentity<float>`)
  instead of instantiating a duplicate `clAdditiveIdentity__f32` class whose
  static `Value()` body was never emitted.  Both the substitution-time and
  compiled-spec argument parsers apply the canonicalization.
- `typename Nested<T>::type` template arguments are parsed (the `typename`
  prefix is skipped and a trailing `::Member` resolves to the member type
  token) instead of failing with `'>' expected after substituted template
  argument`.
- `gen_inline_functions` also flushes pending template specializations queued
  by replayed member bodies (free-template bodies such as `clCopyAssign`
  called from constructors), not just pending member functions.
- Member-template type-argument inference keeps the class's own type-argument
  token when the inferred scalar is compatible, so member bodies construct
  `clMatrix4x4__f32` instead of a duplicate `clMatrix4x4__float`.

New passing regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_remove_reference_typedef_arg_matches_specialization.cpp
```

Suite comparison: Templates 147/16, Constructors 31/0, Classes 50/6,
MemberFunctions 34/2, Destructors 14/0, OperatorOverloads 16/9, All 18/2 —
unchanged from the earlier wave, no regressions.

Measurement: the cl2DDraw, matrix Identity and `clOne`/`clZero` probes now
compile and link (the float `clAdditiveIdentity/...::Value` symbols are
resolved).  The clean Racer split link remains at 37 undefined symbols; the
remaining set is dominated by out-of-class lifecycle/operator bodies not yet
emitted per-TU, CommonLib sources that do not compile yet, and runtime/OS
symbols.  No Racer executable exists yet.

## 2026-08-20 cl2DDraw.cpp:145 Static-Member/Return-Type Replay Wave

The `cl2DDraw.cpp:145 cannot convert 'int' to 'struct clMatrix4x4__f32'`
blocker is fixed.  Root causes found and fixed:

- `class_has_static_member_func()` scanned member-template declarations for a
  `static` keyword using `tok >= TOK_UIDENT`, but `static` is a keyword token
  below that boundary, so inline static member templates were misclassified as
  instance members.  The scan now accepts `TOK_STATIC` directly, and only
  inspects the declaration portion before the body (captured declarations can
  contain stray keyword artifacts inside their bodies, which misclassified
  plain members such as `clList::Erase` as static and dropped their `this`
  parameter).
- Replayed static member and member-template bodies qualify nested static
  calls as `Class::Member` (instead of `this->Member`, which cannot reach
  static members).  `add_pending_member_func`, `add_pending_static_member_func`
  and the template-member body builder all apply the right qualification, and
  constructor spellings (`Class<T>(...)`) are no longer rewritten as static
  member calls.
- Member-template return types are rebuilt without line-info tokens
  (`TOK_LINENUM` pairs inserted by `tok_str_add_tok` used to corrupt
  `Box < float >` into `Box < float <linenumber> >`), parsed into the overload
  metadata so calls see the real struct return instead of `void`, and
  auto-return members whose body returns a same-class member-template call
  (e.g. `Translated` returning `CreateMatrix(...)`) deduce their return type
  from that helper.
- `make_func_type_from_saved_params` marks prototyped signatures with
  `FUNC_NEW` so a later out-of-class definition is not rejected as an
  "incompatible types for redefinition".
- Free-function/static-member overload mangling only protects compiler-internal
  names that *start* with `__`; class-template instance names such as
  `clMatrix4x4__f32_CreateMatrix` now get per-signature suffixes, so the
  16-argument and 4-vector `CreateMatrix` overloads no longer collapse onto one
  symbol.
- Out-of-class plain members with qualified return types (`const
  clVector4<T>& W()`) are parsed correctly by only treating an identifier as
  the method name when it is directly followed by `(`.
- `gen_inline_functions` flushes newly queued member bodies inside its emission
  loop (not just once after it), so bodies instantiated by an emitted inline
  member are themselves emitted.
- Braced initialization materializes out-of-class class-template
  `std::initializer_list` constructors (they were referenced but never
  instantiated, leaving undefined ctor symbols).
- Range-for variable declarations accept real parsed types after `const`
  (`const float &v` previously failed because `float` is a keyword token).

New passing regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_static_member_template_called_from_static_member.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_static_member_calls_auto_return_member_template.cpp
```

Suite comparison against the pre-wave compiler: Templates 145/18 -> 147/16
(two net fixes, zero regressions); Constructors 31/0, Classes 50/6,
MemberFunctions 34/2, Destructors 14/0, OperatorOverloads 16/9, All 18/2,
c_compat 17/0 and MultiSource all unchanged.

Measurement: all 22 generated core sources compile independently, and the
full Racer core build now compiles every source cleanly (previously blocked at
`cl2DDraw.cpp:145`).  The clean split link (22 core objects + retained CommonLib
objects + SDL2 DLL + system libs) is down from 49 to 37 undefined symbols,
resolving all static member-template calls, matrix operator/member bodies and
the `clList` helper chain.  Remaining undefined symbols are dominated by
lifecycle/operator bodies of class templates still not emitted per-TU
(`clList`/`clImage`/`clMat4` constructors, `clVector2::Length`, `operator=`
bodies), explicit-specialization static members reached through deep
template chains (`clAdditiveIdentity/...::Value`), CommonLib sources that do not
compile yet (`clRenderObject.cpp`, `clCamera.cpp`, `clFolder.cpp`,
`clDecryptThis.cpp`, `clStringEncoding.cpp`, stream sources), and runtime/OS
symbols (`alloca`, `_clRelFail`, `GlobalMemoryStatusEx`,
`SetProcessDpiAwareness`, `ImmAssociateContext`, `GetScaleFactorForMonitor`,
`clWindowCreateFlags`, `VertexType`, `clContains`, `clColor_UnpackVec3`).
No Racer executable exists yet; the next blocker class is out-of-class
lifecycle/operator member emission and the CommonLib source compile failures.

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

## 2026-08-20 Pending-Template-Spec Corruption Wave

The `clMatrix4x4`/`clQuaternion` replay heap corruption is fixed and reduced to
a clean compiler diagnostic.

Root causes found and fixed:

- `compile_pending_template_specs()` freed each replay `TokenString` through
  `end_macro()` but left the pointer in `pending_template_specs`, so later
  scans read freed (and often reused) memory. Compiled slots are now set to
  `NULL`, all pending-spec scans skip `NULL`, and un-compiled entries are
  released at template-state teardown.
- Several replay buffers were freed twice because `end_macro()` already owns
  them: explicit function/class template specializations, `auto` initializer
  replay in `decl`, constructor member-initializer argument replay, and braced
  class-return replay.
- Class-template substitution rewrote any global struct typedef identifier,
  even when it was a parameter name. `clMatrix4x4<T>` has a parameter named
  `m4`, which collided with the global `typedef clMatrix4x4<f32> m4;` and
  produced `struct clMatrix4x4__f32` inside the 16-argument constructor's
  parameter list. Typedef tokens are now rewritten only in type positions.

Regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_template_typedef_param_name_collision.cpp
```

The focused test fails on the pre-wave compiler with
`',' expected (got 'Mat4__f32')` and passes with the rebuilt compiler.

Verification:

- `cl2DDraw.cpp` no longer exits with heap corruption; it now reports the next
  clean blocker at `cl2DDraw.cpp:145` (`cannot convert 'int' to
  'struct clMatrix4x4__f32'`).
- All 22 generated core objects compile with the new compiler, including
  `Racer.cpp`.
- Templates suite: 146 passed / 16 failed (pre-wave 145 / 17; the new typedef
  collision test is the net fix, with no regressions). Constructors 31/0,
  Destructors 14/0, MemberFunctions 34/2 unchanged, MultiSource 6/6.
- No Racer executable exists yet; the next measurement blocker is the
  `cl2DDraw.cpp:145` conversion error.

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

The `cl2DDraw.cpp:145 cannot convert 'int' to 'struct clMatrix4x4__f32'`
blocker is fixed; see the 2026-08-20 wave notes above.  The full Racer core
build now compiles every generated source, and the split link is down to 37
undefined symbols.  The next blocker class is out-of-class lifecycle/operator
member-template emission and the CommonLib source compile failures listed above.

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
- The real CommonLib `src/Platform/clAssets.cpp`, added to the core measurement
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

The generated core-only source list still omits these CommonLib implementations,
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
`static_assert`. It still lacks the omitted CommonLib runtime implementations and
several template/lifecycle definitions, so there is still no Racer executable.

### Boolean specialization and CommonLib source progress

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

The full core-only link still omits the CommonLib translation units and reports
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
  stage. Adding CommonLib sources to the same compiler invocation still exposes
  duplicate generated vector constructors, so no Racer executable exists yet.

### CommonLib object and default-argument progress

- All 22 generated core sources compile successfully as independent objects.
  An object-only link reproduces the unresolved-symbol list without the
  same-session duplicate-template definitions, providing a viable route for
  adding CommonLib implementation objects incrementally.
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
  state, monitor scaling, and older OpenGL constants exercised by
  `clWindow.cpp`, with focused passing header tests.
- After correcting a one-underscore `declspec` typo and making three intended
  `clString` converting returns explicit in CommonLibrary, standalone
  `clWindow.cpp` compiles successfully to
  `C:\Luke\Src\OT\cl\builds\split\commonlib_window.obj`.
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
- CPC, all 22 core measurement objects, and all retained CommonLib/helper objects
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

## 2026-08-19 Out-of-Class Template Member Emission Wave

Implemented and verified:

- Used out-of-class class-template member definitions (the `clList.inl` /
  `clVector2.inl` shape) are now emitted per translation unit. The old
  `duplicate_member` guard dropped every body whose mangled symbol already
  existed from the in-class declaration, leaving used members undefined at
  link. It now drops a body only when the existing symbol is already defined
  (non-`extern`), with conservative skips for `operator=` and `Insert` bodies
  whose replay still trips a deeper template-body bug.
- Fixed a use-after-free in the pending-member queue: compiled/dropped member
  slots now become `NULL` (and the compile loop skips `NULL` slots) so nested
  scans of the queue cannot read a token string freed by `end_macro()`.
- The array-bracket replay guard now looks at the signature only, so members
  like `At` whose bodies use `m_pData[index]` are no longer silently skipped.
- Overload-driven instantiation now consults the declared overload table
  (default arguments live in the declaration) and the call's argument types,
  so calls with omitted defaults (e.g. `Reserve(8)`) instantiate the member
  and unrelated arity matches (e.g. `Erase(c--)` no longer instantiates
  `Erase(clList<i64>&)`) are avoided.
- `std::initializer_list`-signature members are only instantiated for calls
  that actually pass an initializer_list, avoiding broken replay of unrelated
  arity matches.

New passing regression coverage:

```bat
Tests\test_MultiSource.cmd -CompilerPath <cpc>
```

The new multi-source test (`test_out_of_class_template_member.*` plus two TUs
sharing the same specialization) fails to link with the previous compiler
(undefined `OutOfClassBox__*` members) and passes with the new one.

Suite comparison against the pre-wave compiler: Templates 33 -> 26 failures
(7 out-of-class template member tests fixed, zero regressions), all other
suites unchanged. All 22 core objects, `clString.cpp`, `clControls.cpp`,
`clAssets.cpp`, `cl2DDraw*`, `clWindow.cpp`, `clScan.cpp` and the remaining
CommonLib helpers compile with the new compiler.

Measurement: the clean Racer split link (22 core + CommonLib objects + SDL2
import + system libs) dropped from 143 to 133 unresolved symbols, resolving
71 baseline symbols. The remaining undefined set is dominated by the
`Insert`/`Move`/`Realloc`/`TryReserve`/`Capacity` helper chain (their bodies
are still not replayed from within other template member bodies) plus the
additional `clList<float>`/`clList<i64>` references introduced by
`clScan.cpp`. No Racer executable exists yet.

## 2026-08-19 Helper-Chain Replay Wave

Implemented and verified:

- Member overload resolution now accepts standard scalar conversions
  (`int` to `i64`, etc.) when matching saved call arguments, with a distinct
  rank below exact matches. This fixes multi-arity overloads such as
  `Insert(i64, const T*, i64)` being rejected for `l.Insert(0, p, 3)`.
- Free function templates record both minimum and maximum call arity, so a
  defaulted third parameter (e.g. `clMoveConstruct(..., count = 1)`) matches
  two- and three-argument calls, while a one-parameter template no longer
  matches a two-argument call.
- Variadic function templates are registered even when their pack lowers the
  required type-parameter count, and call inference now carries inferred pack
  element types into the instantiated body. Single-element forwarding such as
  `clConstruct(p, clForward(Args, args)...)` compiles and links.
- The `instantiate_template_member_for_call` guard is fixed so helper members
  (`Grow`, `Move`, `TryReserve`, `Realloc`, `Capacity`, `Data`, etc.) are
  instantiated from inside other non-lifecycle template member bodies.
- The conservative `Insert` body skip is removed; `Insert` definitions now
  replay and emit.
- Overload selection among same-arity function templates prefers a template
  whose first parameter is compatible with the call argument, preventing the
  scalar `clMin` call from matching the `clVector2<T>` overload.
- CPC runtime `std::forward`/`std::move` return the reference parameter
  directly instead of using the unsupported `(T&&)value` C-style cast.
- CommonLibrary working tree: `clMoveAssign` and `clDestruct` gained the
  intended `= 1` default so their one-element calls match (same class of
  defect as the earlier `clMoveConstruct` fix).

New passing regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_template_member_helper_replay.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_variadic_template_pack_inference.cpp
```

Suite comparison: Templates 126 passed / 25 failed versus the pre-wave
compiler's 124 passed / 27 failed (two net fixes, no regressions); Classes
45/11 and All 17/3 unchanged; MultiSource passes all six groups.

Measurement: all 22 core objects and all retained CommonLib/helper objects
recompile with the new compiler. The clean Racer split link dropped from 133
to 89 unresolved symbols; the `Insert`/`Move`/`Realloc`/`TryReserve`/
`Capacity` helper chain is gone from the undefined set. Remaining unresolved
work is dominated by out-of-class member-template bodies such as
`clVector2<T>::operator-` being lost during pending-member queue compaction,
the still-skipped variadic member `Resize`, missing CommonLib implementation
objects (`clFolder`, `clDecryptThis`, `clStringEncoding`, stream readers),
and a few runtime symbols (`alloca`, `GlobalMemoryStatusEx`, `_clRelFail`).
No Racer executable exists yet.

## 2026-08-19 Constructor/Queue/Operator/Destructor Wave

Implemented and verified:

- `compile_pending_member_funcs` is now re-entrant. A nested full-queue flush
  (triggered by constructor resolution inside a replayed member body) used to
  compact the shared queue while the outer pass was mid-iteration and dropped
  entries the outer pass still owed; out-of-class template operator bodies
  such as `clVector2<T>::operator-` were being lost at link. Nested flushes
  now return immediately and the outer pass picks up appended entries via its
  live loop bound.
- Saved-parameter signatures (`make_func_type_from_saved_params`) now create
  parameters with `VT_LOCAL | VT_LVAL` instead of `r = 0`. Overload metadata
  reuses that chain through `use_overload_func_type()`, so replayed
  constructor/function bodies read parameter values instead of taking
  addresses. This restores member-initializer stores (`x(px), y(py)`) and
  constructor parameter reads for float/double/long-long/unsigned params.
- Class placement-new (`new (storage) Class(args)`) now routes through the
  constructor path whenever the class declares any lifecycle constructor,
  instead of requiring a zero-argument constructor. Placement new with
  non-default constructors works.
- Binary operator member calls (`a - b` through `operator-(const Vec2<U>&)`)
  now type the rhs argument with `gfunc_param_typed` before the call, so a
  struct rhs is passed by reference instead of by value. The real
  `clVector2<T>` operator overloads now link and run.
- Out-of-class template destructor definitions are instantiated when a
  class-template instance is destroyed at scope exit (`try_call_scope_cleanup`)
  and `template_member_def_param_count` treats destructors as zero-parameter,
  so `clList<X>_destructor`, `Box__int_destructor`, etc. are emitted per TU.

New passing regression coverage:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_template_out_of_class_destructor_definition.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_vector_operator_vector_overload.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_nested_out_of_class_member_call.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_vec2_scalar_operator.cpp
```

Suite comparison against the pre-wave compiler (90afcdc): Constructors
22 -> 31 passed (9 constructor tests fixed: inline member initializer lists,
overloaded member-initializer constructors, class placement new, copy
constructors with owning members, array-reference constructor parameters);
Templates 127 -> 135 passed (8 net fixes, zero regressions); Classes
45 -> 50 passed; MemberFunctions 34/2 and OperatorOverloads 16/9 unchanged;
MultiSource passes all six groups. All 22 core objects and all retained
CommonLib/helper objects compile with the new compiler.

Measurement: the clean Racer split link dropped from 89 to 49 unresolved
symbols. The real `clVector2<float>` operator probe (`a - b`, `Zero`/`Length`
shapes) now compiles, links, and runs with correct values. The clList
destructor family is gone from the undefined set. Remaining unresolved work is
dominated by static member-template calls (`clVector2<T>::Zero()`, `One()`),
non-static template members that still need call-site instantiation
(`operator[]`, `Length`, matrix members), missing CommonLib implementation
objects (`clFolder`, `clDecryptThis`, `clStringEncoding`, stream readers,
`clRenderObject` methods), and runtime/import symbols (`alloca`,
`GlobalMemoryStatusEx`, `_clRelFail`, `LoadGLFunctions`). No Racer executable
exists yet.
