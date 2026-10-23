param(
    [ValidateSet('3','4')][string]$Version = '4',
    [string]$OpenScad = '',
    [string]$OutputDirectory = '',
    [string]$Python = 'python',
    [string[]]$Define = @()
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $projectRoot "hardware/enclosure/enclosure_v$Version.scad"
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot "build/enclosure-v$Version" }
if (-not $OpenScad) {
    $command = Get-Command openscad.com -ErrorAction SilentlyContinue
    if ($command) { $OpenScad = $command.Source }
    else { $OpenScad = Join-Path $env:ProgramFiles 'OpenSCAD/openscad.com' }
}
if (-not (Test-Path -LiteralPath $OpenScad)) { throw 'OpenSCAD CLI not found. Supply -OpenScad with its path.' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

function Export-Cad([string]$Name, [string]$Extension, [string[]]$ExtraArguments) {
    $target = Join-Path $OutputDirectory "$Name.$Extension"
    $arguments = @('-o', $target, '--hardwarnings', '-D', "part=`"$Name`"")
    if ($Extension -eq 'stl') { $arguments += @('--export-format', 'binstl') }
    else { $arguments += @('--colorscheme', 'Metallic') }
    foreach ($definition in $Define) { $arguments += @('-D', $definition) }
    $arguments += $ExtraArguments
    $arguments += $source
    Write-Host "Exporting $Name.$Extension"
    $log = & $OpenScad @arguments 2>&1
    $exitCode = $LASTEXITCODE
    $log | ForEach-Object { $_.ToString() } | Set-Content -LiteralPath (Join-Path $OutputDirectory "$Name-$Extension.log")
    if ($exitCode -ne 0 -or ($log -match 'WARNING:|ERROR:') -or -not (Test-Path -LiteralPath $target)) {
        $log | ForEach-Object { Write-Host $_ }
        throw "OpenSCAD failed for $Name.$Extension. See export log."
    }
    if ($Extension -eq 'stl') {
        & $Python (Join-Path $PSScriptRoot 'clean_enclosure_stl.py') $target
        if ($LASTEXITCODE -ne 0) { throw "STL validation failed for $Name" }
    }
}

$printParts = @('front', 'back', 'clamps', 'fit_coupon')
if ($Version -eq '4') { $printParts += @('antenna_retainers', 'speaker_retainer') }
foreach ($name in $printParts) { Export-Cad $name 'stl' @() }
if ($Version -eq '4') {
    & $Python (Join-Path $PSScriptRoot 'layout_enclosure_plate.py') --parts $OutputDirectory --output (Join-Path $OutputDirectory 'enclosure-v4-all-parts-180mm.stl')
    if ($LASTEXITCODE -ne 0) { throw '180 mm print plate layout or validation failed' }
}
Export-Cad 'assembled' 'png' @('--imgsize', '1400,1100', '--camera', '175,-180,-220,50,56,20', '--viewall', '--autocenter')
Export-Cad 'exploded' 'png' @('--imgsize', '1400,1100', '--camera', '180,-190,-150,50,56,30', '--viewall', '--autocenter')
Export-Cad 'back' 'png' @('--imgsize', '1400,1100', '--camera', '185,-180,220,50,56,10', '--viewall', '--autocenter')
Export-Cad 'front' 'png' @('--imgsize', '1400,1100', '--camera', '185,-180,220,50,56,10', '--viewall', '--autocenter')
Write-Host "Enclosure exports: $OutputDirectory"
Write-Host 'These are CAD exports. Confirm measurements and print the fit coupon before a full enclosure.'
