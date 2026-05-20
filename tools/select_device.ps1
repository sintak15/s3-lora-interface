param(
  [Parameter(Mandatory = $true)]
  [ValidateSet('jjs-node', 'babs-node')]
  [string] $Name,

  [string] $ApPass = "12345678",

  [string] $WebPass = "admin"
)

$channels = @{
  'jjs-node' = 6
  'babs-node' = 11
}

$configDir = Join-Path $PSScriptRoot '..\config'
$configPath = Join-Path $configDir 'device_config.local.h'
New-Item -ItemType Directory -Force $configDir | Out-Null

$channel = $channels[$Name]
@"
#pragma once

#define DEVICE_NAME "$Name"
#define DEVICE_HOSTNAME DEVICE_NAME
#define INTERFACE_AP_SSID DEVICE_NAME
#define INTERFACE_AP_PASS "$ApPass"
#define WIFI_AP_CHANNEL $channel
#define WEBUI_AUTH_PASS "$WebPass"
"@ | Set-Content -LiteralPath $configPath -Encoding ascii

Write-Host "Selected $Name on AP channel $channel"
Write-Host "Wrote $configPath"
