param(
  [ValidateSet('current', 'universal')]
  [string] $Target = 'current',

  [string] $Name = '',

  [switch] $NoVersionBump
)

$ErrorActionPreference = 'Stop'

$repo = Resolve-Path (Join-Path $PSScriptRoot '..')
$constantsPath = Join-Path $repo 'constants.h'
$localConfig = Join-Path $repo 'config\device_config.local.h'
$localConfigBackup = Join-Path $repo 'config\device_config.local.h.codexbak'
$buildPath = Join-Path $repo "build\$Target"
$distPath = Join-Path $repo 'dist'
$arduinoCli = Join-Path $env:LOCALAPPDATA 'Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$fqbn = 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc,DebugLevel=info'

$constants = Get-Content -Raw -LiteralPath $constantsPath
$match = [regex]::Match($constants, '#define\s+FIRMWARE_VERSION\s+"v(\d+)\.(\d+)\.(\d+)"')
if (-not $match.Success) {
  throw 'Could not find FIRMWARE_VERSION in constants.h'
}

$major = [int]$match.Groups[1].Value
$minor = [int]$match.Groups[2].Value
$patch = [int]$match.Groups[3].Value
if (-not $NoVersionBump) {
  $patch++
  $newVersion = "v$major.$minor.$patch"
  $constants = [regex]::Replace($constants, '#define\s+FIRMWARE_VERSION\s+"v\d+\.\d+\.\d+"', "#define FIRMWARE_VERSION `"$newVersion`"", 1)
  Set-Content -LiteralPath $constantsPath -Value $constants -Encoding ascii
} else {
  $newVersion = "v$major.$minor.$patch"
}

New-Item -ItemType Directory -Force $distPath | Out-Null

try {
  if ($Target -eq 'universal' -and (Test-Path -LiteralPath $localConfig)) {
    if (Test-Path -LiteralPath $localConfigBackup) {
      Remove-Item -LiteralPath $localConfigBackup -Force
    }
    Rename-Item -LiteralPath $localConfig -NewName 'device_config.local.h.codexbak'
  }

  & $arduinoCli compile --fqbn $fqbn --build-path $buildPath $repo
  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }
} finally {
  if (Test-Path -LiteralPath $localConfigBackup) {
    Rename-Item -LiteralPath $localConfigBackup -NewName 'device_config.local.h'
  }
}

$date = Get-Date -Format 'yyyyMMdd'
$label = if ($Name) { $Name } else { $Target }
$out = Join-Path $distPath "s3-lora-interface-$label-$newVersion-$date.bin"
Copy-Item -LiteralPath (Join-Path $buildPath 's3-lora-interface.ino.bin') -Destination $out -Force
Get-Item -LiteralPath $out | Select-Object FullName, Length, LastWriteTime
