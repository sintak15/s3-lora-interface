param()
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$commit = & git -C $repo rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-f]{40}$') { throw 'Cannot identify source commit.' }
$changes = & git -C $repo status --porcelain --untracked-files=normal
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect source changes.' }
$dirty = if (@($changes).Count -gt 0) { 'true' } else { 'false' }
$content = "#pragma once`n#define FIRMWARE_GIT_COMMIT `"$commit`"`n#define FIRMWARE_SOURCE_DIRTY $dirty`n"
$path = Join-Path $repo 'config/build_identity.generated.h'
if (!(Test-Path -LiteralPath $path) -or (Get-Content -Raw -LiteralPath $path) -ne $content) {
  [IO.File]::WriteAllText($path, $content, [Text.Encoding]::ASCII)
}
Write-Host "Build source: $commit (dirty=$dirty)"
