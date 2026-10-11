param(
  [string] $ArduinoCli = '',
  [string] $Node = 'node'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $ArduinoCli) {
  $command = Get-Command arduino-cli -ErrorAction SilentlyContinue
  if ($command) { $ArduinoCli = $command.Source }
  else { $ArduinoCli = Join-Path $env:LOCALAPPDATA 'Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe' }
}
& (Join-Path $PSScriptRoot 'check_build_environment.ps1') -ArduinoCli $ArduinoCli
& $Node (Join-Path $repo 'tests/webui.test.js')
if ($LASTEXITCODE -ne 0) { throw 'Web UI regression checks failed.' }
$lock = Get-Content -Raw -LiteralPath (Join-Path $repo 'config/build-dependencies.json') | ConvertFrom-Json
& $ArduinoCli compile --fqbn $lock.fqbn --build-path (Join-Path $repo 'build/verify') --build-property 'compiler.cpp.extra_flags=-DFIRMWARE_RUN_TESTS' $repo
if ($LASTEXITCODE -ne 0) { throw 'Firmware compilation or compile-time regression checks failed.' }
Write-Host 'Firmware build and regression checks passed. Hardware testing remains necessary.'
