<#
serial.ps1 - save raw bytes from a serial port to a file, optionally sending a command file first.

Uses only the .NET SerialPort class that ships with Windows PowerShell; nothing to install.
Not yet verified against real hardware: confirm the COM port and baud with an ordinary
serial terminal first. The file is kept ASCII-only so Windows PowerShell 5.1 parses it
the same way regardless of the system code page.

Usage:
  powershell -ExecutionPolicy Bypass -File serial.ps1 -List
  powershell -ExecutionPolicy Bypass -File serial.ps1 -Port COM5 -Seconds 60 -Out capture.bin
  powershell -ExecutionPolicy Bypass -File serial.ps1 -Port COM5 -Seconds 5 -Out reply.bin -Send cmd.bin
Then analyse with: ./rx_check capture.bin
#>
param(
    [string]$Port,
    [int]$Baud = 115200,
    [int]$Seconds = 10,
    [string]$Out = 'capture.bin',
    [string]$Send,
    [switch]$List
)

if ($List) {
    [System.IO.Ports.SerialPort]::GetPortNames()
    return
}
if (-not $Port) {
    Write-Error 'Specify -Port, for example -Port COM5. Use -List to see available ports.'
    exit 2
}

$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$sp.Handshake = [System.IO.Ports.Handshake]::None
$sp.Open()
try {
    if ($Send) {
        $bytes = [System.IO.File]::ReadAllBytes((Resolve-Path $Send).Path)
        $sp.Write($bytes, 0, $bytes.Length)
    }
    $file = [System.IO.File]::Create((Join-Path (Get-Location) $Out))
    try {
        $buf = New-Object byte[] 4096
        $total = 0
        $deadline = (Get-Date).AddSeconds($Seconds)
        while ((Get-Date) -lt $deadline) {
            $avail = $sp.BytesToRead
            if ($avail -gt 0) {
                $n = $sp.Read($buf, 0, [Math]::Min($avail, $buf.Length))
                $file.Write($buf, 0, $n)
                $total += $n
            } else {
                Start-Sleep -Milliseconds 5
            }
        }
    } finally {
        $file.Close()
    }
    Write-Host "received $total bytes into $Out"
} finally {
    $sp.Close()
}
