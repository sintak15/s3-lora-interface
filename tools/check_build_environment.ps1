param(
  [string] $ArduinoCli = ''
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$lock = Get-Content -Raw -LiteralPath (Join-Path $repo 'config/build-dependencies.json') | ConvertFrom-Json
if (-not $ArduinoCli) {
  $command = Get-Command arduino-cli -ErrorAction SilentlyContinue
  if ($command) { $ArduinoCli = $command.Source }
  else { $ArduinoCli = Join-Path $env:LOCALAPPDATA 'Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe' }
}
if (-not (Test-Path -LiteralPath $ArduinoCli)) { throw 'arduino-cli was not found; pass -ArduinoCli with its path.' }

$coresJson = & $ArduinoCli core list --format json
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect installed Arduino cores.' }
$cores = $coresJson | ConvertFrom-Json
$core = @($cores.platforms | Where-Object { $_.id -eq $lock.core.id })
if ($core.Count -ne 1 -or $core[0].installed_version -ne $lock.core.version) {
  throw "Install the pinned core: arduino-cli core install $($lock.core.id)@$($lock.core.version)"
}
$librariesJson = & $ArduinoCli lib list --format json
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect installed Arduino libraries.' }
$libraries = @((($librariesJson | ConvertFrom-Json).installed_libraries) | ForEach-Object { $_.library })
foreach ($entry in $lock.libraries.PSObject.Properties) {
  $matches = @($libraries | Where-Object { $_.name -eq $entry.Name -and $_.version -eq $entry.Value })
  if ($matches.Count -ne 1) { throw "Install the pinned library: arduino-cli lib install `"$($entry.Name)@$($entry.Value)`"" }
}
foreach ($entry in $lock.nanopb.sha256.PSObject.Properties) {
  $file = Join-Path (Join-Path $repo $lock.nanopb.source_dir) $entry.Name
  if (-not (Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $entry.Value) {
    throw "Nanopb runtime differs from the pinned snapshot: $($entry.Name). See docs/BUILD.md."
  }
}
Write-Host 'Build dependencies match config/build-dependencies.json.'
