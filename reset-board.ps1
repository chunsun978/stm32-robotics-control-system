#!/usr/bin/env pwsh
# Reset STM32 board via OpenOCD
# Run this if you need to reset the board before flashing

Write-Host "Resetting STM32 board..." -ForegroundColor Cyan

openocd -f interface/stlink.cfg -f target/stm32f4x.cfg `
    -c "init" `
    -c "reset halt" `
    -c "reset run" `
    -c "exit"

Write-Host "Board reset complete!" -ForegroundColor Green

