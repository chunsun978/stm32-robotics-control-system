#!/usr/bin/env pwsh
# Simple serial monitor for STM32 UART output

param(
    [string]$Port = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "STM32 Serial Monitor" -ForegroundColor Cyan
Write-Host "Connecting to $Port at $BaudRate baud..." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to exit" -ForegroundColor Yellow
Write-Host "-----------------------------------" -ForegroundColor Cyan

try {
    $port = new-Object System.IO.Ports.SerialPort $Port,$BaudRate,None,8,one
    $port.Open()
    
    Write-Host "Connected! Listening for data..." -ForegroundColor Green
    Write-Host ""
    
    while($true) {
        if ($port.BytesToRead -gt 0) {
            $data = $port.ReadExisting()
            Write-Host $data -NoNewline
        }
        Start-Sleep -Milliseconds 10
    }
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Available COM ports:" -ForegroundColor Yellow
    [System.IO.Ports.SerialPort]::getportnames()
    Write-Host ""
    Write-Host "Usage: .\serial-monitor.ps1 -Port COM3 -BaudRate 115200" -ForegroundColor Cyan
}
finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
    }
}

