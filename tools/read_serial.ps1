param([string]$portName = "COM5", [int]$baudRate = 115200)
$port = New-Object System.IO.Ports.SerialPort $portName, $baudRate, None, 8, One
$port.DtrEnable = $false
$port.RtsEnable = $true
$port.Open()
Start-Sleep -Milliseconds 100
$port.DtrEnable = $false
$port.RtsEnable = $false
Start-Sleep -Milliseconds 6000
$out = $port.ReadExisting()
$port.Close()
Write-Output $out
