# CPrime Racer Compatibility Task

Last updated: 2026-08-21

## Goal

Extend compiler compatibility for Racer until this file exists:

`C:\Luke\Src\OT\cl\builds\racer\Racer.exe`

Use this file to run the current build attempt:

`C:\Luke\Src\CPrime\build_racer.ps1`

Do not call Racer ready to test until `Racer.exe` exists at the path above.

## Current State

- `C:\Luke\Src\CPrime\cpc.exe` rebuilds successfully.
- `C:\Luke\Src\OT\cl\builds\racer\Racer.exe` is not produced yet.
- Latest verified compiler progress: fixed qualified template-base member calls
  like `clArray2<ui32>::Size()` and `clArray2<ui32>::Data()` from `clImage`.
- New verified compiler progress: fixed class-template forward completion when
  template friend declarations mention the same class template, and allowed
  repeated class-template forward declarations after a completed definition.
  This clears the `clByteList` / `clList<char>` incomplete-field failures in
  `clMemoryStream.h` and `clString.h`.
- New verified compiler progress: fixed qualified static member calls such as
  `clMat4::Translation(clVec3(...))` inside nested functional constructor
  arguments.  This clears `additional_cl2DDraw`; `additional_clWindow` was
  also rechecked and now compiles.
- New verified compiler progress: fixed nested constructor argument conversion
  for return/member-initializer paths, class-scope enum/static-const default
  replay, out-of-class operator-body member calls, and scoped initializer
  parameter inference.  This moved `additional_clFolder` past the previous
  `wchar_t[260]` to `clPath`, `clFST_None`, `m_path`, `npos`, `wstring`, and
  `Close()` diagnostics.
- Current `build_racer.ps1` result:
  - generated source files compile.
  - additional source files still have compile failures.
  - link fails.

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

Surrounding suite counts after the last compiler change:

- `Tests\features\Constructors`: 35 passed, 0 failed.
- `Tests\features\Classes`: 57 passed, 6 failed.
- `Tests\features\Templates`: 166 passed, 14 failed.

## Current Compile Failures From `build_racer.ps1`

- `additional_clPath`: `Platform/clPath.cpp:235`, cannot convert
  `const clPath *`.
- `additional_clFolder`: currently exits with access violation
  `-1073741819` after clearing the earlier typed diagnostics listed above.
- `additional_clDecryptThis`: `Encryption/clDecryptThis.cpp:61`, incompatible
  types in the `Cypher` / `clList<Atom>` path.
- `additional_clCamera`: `UI/clCamera.cpp:9`, cannot convert `int` to the
  expected vector/class type.
- `additional_clImage`: `Raster/clHistogram.h:9`, `',' expected`.
- `additional_clRenderObjectCore`: `Polygon/clRenderObjectCore.cpp:29`,
  cannot convert `char *` to the expected class/reference type.
- `additional_clRenderObject`: `Polygon/clRenderObject.cpp:70`, expression
  expected before `{`.

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

Keep extending compiler compatibility for the Racer build path until:

`C:\Luke\Src\OT\cl\builds\racer\Racer.exe`

exists.
