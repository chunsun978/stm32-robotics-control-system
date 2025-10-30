#!/usr/bin/env pwsh
# Test ST-LINK GDB Server with Admin privileges

# Check if running as admin
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "Restarting with Administrator privileges..." -ForegroundColor Yellow
    Start-Process pwsh -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

$gdbServerPath = "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_2.2.200.202505060755\tools\bin\ST-LINK_gdbserver.exe"
$cubeProgrammerPath = "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.200.202503041107\tools\bin"

Write-Host "Running as Administrator..." -ForegroundColor Green
Write-Host "Testing ST-LINK GDB Server..." -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop" -ForegroundColor Yellow
Write-Host ""

& $gdbServerPath -p 3333 -cp $cubeProgrammerPath -l 4

Write-Host ""
Write-Host "GDB Server stopped." -ForegroundColor Cyan
Read-Host "Press Enter to close"

