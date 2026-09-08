param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = (Join-Path $PSScriptRoot '../third-party/clang/bin/clang.exe')
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
$sources = Join-Path $PSScriptRoot 'abi\msvc_members'
$commonArgs = @('-Werror')
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-msvc-members-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    $provider = Join-Path $work 'provider.obj'
    & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
        -c (Join-Path $sources 'provider.cpp') -o $provider
    if ($LASTEXITCODE -ne 0) { throw 'Native member provider failed to compile.' }
    $executable = Join-Path $work 'members.exe'
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & $compiler @commonArgs (Join-Path $sources 'consumer.cpp') $provider -o $executable 2>&1
        $result = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($result -ne 0) { throw "Mixed member build failed (exit $result):`n$($output -join "`n")" }
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw 'Member executable missing.' }
    & $executable
    if ($LASTEXITCODE -ne 0) { throw "Mixed member executable returned $LASTEXITCODE." }
    Write-Output 'PASS native/CPC constructors, destructors, operators, access, nested names, template and scalar signatures, callbacks, and member record returns'
    $typedefFixtures = Join-Path $PSScriptRoot 'features\Classes\pass'
    $definition = Join-Path $typedefFixtures 'typedef_record_linkage_other.cpp'
    $caller = Join-Path $typedefFixtures 'test_typedef_record_linkage_across_inputs.cpp'
    foreach ($nativeDefinition in @($true, $false)) {
        $nativeSource = if ($nativeDefinition) { $definition } else { $caller }
        $primeSource = if ($nativeDefinition) { $caller } else { $definition }
        $nativeObject = Join-Path $work 'typedef-native.obj'
        & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
            -c $nativeSource -o $nativeObject
        if ($LASTEXITCODE -ne 0) { throw 'Native typedef record fixture failed to compile.' }
        $typedefExecutable = Join-Path $work 'typedef-records.exe'
        & $compiler @commonArgs $primeSource $nativeObject -o $typedefExecutable
        if ($LASTEXITCODE -ne 0) { throw 'Mixed typedef record fixture failed to link.' }
        & $typedefExecutable
        if ($LASTEXITCODE -ne 0) { throw "Mixed typedef record fixture returned $LASTEXITCODE." }
    }
    Write-Output 'PASS unnamed typedef enums and records, nested names, and aliases in both native/CPC directions'
    $characterFixtures = Join-Path $PSScriptRoot 'features\Templates\pass'
    $definition = Join-Path $characterFixtures 'utf_character_linkage_other.cpp'
    $caller = Join-Path $characterFixtures 'test_utf_character_linkage_across_inputs.cpp'
    foreach ($nativeDefinition in @($true, $false)) {
        $nativeSource = if ($nativeDefinition) { $definition } else { $caller }
        $primeSource = if ($nativeDefinition) { $caller } else { $definition }
        $nativeObject = Join-Path $work 'character-native.obj'
        & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
            -c $nativeSource -o $nativeObject
        if ($LASTEXITCODE -ne 0) { throw 'Native character fixture failed to compile.' }
        $characterExecutable = Join-Path $work 'characters.exe'
        & $compiler @commonArgs $primeSource $nativeObject -o $characterExecutable
        if ($LASTEXITCODE -ne 0) { throw 'Mixed character fixture failed to link.' }
        & $characterExecutable
        if ($LASTEXITCODE -ne 0) { throw "Mixed character fixture returned $LASTEXITCODE." }
    }
    Write-Output 'PASS distinct wchar_t, char16_t, char32_t scalar/reference/template signatures in both native/CPC directions'
    $definition = Join-Path $characterFixtures 'function_address_linkage_other.cpp'
    $caller = Join-Path $characterFixtures 'test_function_address_linkage_across_inputs.cpp'
    foreach ($nativeDefinition in @($true, $false)) {
        $nativeSource = if ($nativeDefinition) { $definition } else { $caller }
        $primeSource = if ($nativeDefinition) { $caller } else { $definition }
        $nativeObject = Join-Path $work 'address-native.obj'
        & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
            -c $nativeSource -o $nativeObject
        if ($LASTEXITCODE -ne 0) { throw 'Native function address fixture failed to compile.' }
        $addressExecutable = Join-Path $work 'addresses.exe'
        & $compiler @commonArgs $primeSource $nativeObject -o $addressExecutable
        if ($LASTEXITCODE -ne 0) { throw 'Mixed function address fixture failed to link.' }
        & $addressExecutable
        if ($LASTEXITCODE -ne 0) { throw "Mixed function address fixture returned $LASTEXITCODE." }
    }
    Write-Output 'PASS function pointer template argument linkage in both native/CPC directions'
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-msvc-members-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
