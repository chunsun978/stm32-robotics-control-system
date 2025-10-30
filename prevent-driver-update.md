# Prevent Windows from Reverting ST-Link Driver

If Windows keeps reverting your WinUSB driver back to STMicroelectronics:

## Option 1: Disable Driver Updates for ST-Link

1. Open Device Manager
2. Find "ST-Link Debug (Interface 0)"
3. Right-click → Properties
4. Driver tab → "Update Driver"
5. "Browse my computer for drivers"
6. "Let me pick from a list of available drivers"
7. Check "Show compatible hardware"
8. Select "USB Serial Device" or "WinUSB Device"
9. Click Next

## Option 2: Disable Automatic Driver Installation

1. Press Win + R, type: `SystemPropertiesAdvanced`
2. Click "Hardware" tab → "Device Installation Settings"
3. Select "No (your device might not work as expected)"
4. Click "Save Changes"

## Option 3: Use Group Policy (Windows Pro/Enterprise)

1. Press Win + R, type: `gpedit.msc`
2. Navigate to: Computer Configuration → Administrative Templates → System → Device Installation → Device Installation Restrictions
3. Enable "Prevent installation of devices that match these device IDs"
4. Add: `USB\VID_0483&PID_374B` (ST-Link VID/PID)

## Quick Fix: Always Check Before Use

Before debugging, always run:
```powershell
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

If it fails with LIBUSB_ERROR_ACCESS, redo Zadig.

