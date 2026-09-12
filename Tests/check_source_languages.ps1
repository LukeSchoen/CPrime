$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$files = @(& git -C $root ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate repository files' }
$source = @('.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx', '.s', '.asm', '.inc', '.inl', '.cmd', '.bat', '.ps1')
$data = @('.md', '.txt', '.expect', '.def', '.json', '.jsonl', '.csv', '.tsv', '.png', '.jpg', '.svg', '.mtl', '.exe', '.dll', '.a', '.lib')
$violations = @()
foreach ($file in ($files | Sort-Object -Unique)) {
    if ($file.StartsWith('third-party/') -or $file.StartsWith('build/') -or
        $file.StartsWith('Tests/pedantic/gcc/corpus/')) { continue }
    $path = Join-Path $root $file
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
    $extension = [IO.Path]::GetExtension($file).ToLowerInvariant()
    $name = [IO.Path]::GetFileName($file)
    if ($extension -notin $source -and $extension -notin $data -and
        $name -notin @('.gitignore', '.gitattributes', '.editorconfig', 'LICENSE', 'COPYING', 'NOTICE') -and
        -not ($extension -eq '' -and $file.StartsWith('include/runtime/'))) {
        $violations += "$file has an unsupported source/file type"
    }
    if ($extension -in @('.ps1', '.cmd', '.bat')) {
        $text = [IO.File]::ReadAllText($path)
        if ($text -match '(?im)^\s*Add-Type\s+-(TypeDefinition|MemberDefinition)\b' -or
            $text -match '(?im)^\s*(?:&\s+|call\s+)?(?:python(?:3|\.exe)?|py(?:\.exe)?|node(?:\.exe)?)\s') {
            $violations += "$file invokes an unsupported language"
        }
    }
}
if ($violations.Count) { throw ($violations -join "`n") }
Write-Output 'PASS first-party source language policy (third-party code and generated build output excluded)'
