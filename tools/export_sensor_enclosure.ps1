param(
    [string]$OpenScad = 'C:/Program Files/OpenSCAD/openscad.com',
    [string]$OutputDirectory = '',
    [string[]]$Parts = @('front','back','gps_retainer','am2302_holder','keyboard_tray','retrofit_gps','retrofit_ads','retrofit_antenna','fit_coupon'),
    [string[]]$Define = @()
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'build/enclosure-sensors' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
foreach ($part in $Parts) {
    if ($part -notmatch '^[a-z0-9_]+$') { throw 'Invalid part name' }
    $target = Join-Path $OutputDirectory "sensors-$part.stl"
    $arguments = @('--hardwarnings','--export-format','binstl','-o',$target,'-D',"revision_part=`"$part`"")
    foreach ($definition in $Define) { $arguments += @('-D',$definition) }
    $arguments += Join-Path $root 'hardware/enclosure/enclosure_v5_sensors.scad'
    $log = & $OpenScad @arguments 2>&1
    $exitCode = $LASTEXITCODE
    $log | ForEach-Object { $_.ToString() } | Set-Content -LiteralPath (Join-Path $OutputDirectory "$part.log")
    if ($exitCode -ne 0 -or ($log -match 'WARNING:|ERROR:')) { throw "OpenSCAD failed: $part" }
    & python (Join-Path $PSScriptRoot 'clean_enclosure_stl.py') $target
    if ($LASTEXITCODE -ne 0) { throw "Mesh validation failed: $part" }
}

