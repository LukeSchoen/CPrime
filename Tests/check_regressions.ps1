param([string]$CompilerPath = '', [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
# General compiler compatibility sentinels; all inputs live in this repository.
$groups = @(
    @{ Suite = 'c_compat'; Tests = @('test_abstract_function_pointer_cast.c',
        'test_winapi_function_pointer_cast.c', 'test_nested_pointer_cast_argument.c') },
    @{ Suite = 'features/Expressions'; Tests = @('test_abstract_function_pointer_cast.cpp',
        'test_parenthesized_functional_construction.cpp') },
    @{ Suite = 'features/Constructors'; Tests = @('test_implicit_derived_copy_with_base_constructors.cpp',
        'test_initializer_list_backing_lifetime.cpp') },
    @{ Suite = 'features/Statements'; Tests = @('test_static_string_array_in_branch.cpp',
        'test_static_string_array_too_long.cpp') }
)
foreach ($group in $groups) {
    & (Join-Path $PSScriptRoot 'run.ps1') -Suite $group.Suite -Select $group.Tests `
        -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
& (Join-Path $PSScriptRoot 'gates.ps1') -Subsystem overloads `
    -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
exit $LASTEXITCODE
