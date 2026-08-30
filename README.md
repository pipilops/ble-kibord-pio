# BLE Kibord — PlatformIO port

Converted from the Arduino IDE sketch `BLE_Kibord.ino` for an ESP32-S3.
This should now build as-is.

## What changed from the .ino

- `BLE_Kibord.ino` → `src/main.cpp`, with `#include <Arduino.h>` added at the
  top (required for a plain `.cpp` file — the Arduino IDE adds this
  automatically for `.ino` files, PlatformIO does not). Sketch logic itself
  is untouched.
- Your `EspUsbHost` / `EspUsbHostKeybord` library (TANAKA Masayuki, v1.0.1)
  is vendored in `lib/EspUsbHost/` using your original source files —
  PlatformIO auto-discovers private libraries dropped into `lib/`, no
  `lib_deps` entry needed for it.

## Things to check on your hardware

1. **Board**: configured for `esp32-s3-devkitc-1` in `platformio.ini`. If
   you're on a different S3 board (Waveshare / Lolin / generic module),
   swap the `board =` line — run `pio boards espressif32 | grep -i s3` to
   list valid IDs.
2. **USB wiring**: `EspUsbHost.h`/`.cpp` talk to the native USB OTG
   peripheral directly via `usb/usb_host.h`. `platformio.ini` sets
   `-DARDUINO_USB_MODE=0 -DARDUINO_USB_CDC_ON_BOOT=0` so the Arduino core
   doesn't also try to claim that peripheral for CDC/Serial. Some S3 boards
   expose two USB connectors (one native OTG, one via a separate UART
   bridge) — make sure the wired keyboard is on the **native USB** port.
3. **BleKeyboard library**: pulled from the registry as
   `t-vk/ESP32 BLE Keyboard @ ^0.3.2` via `lib_deps` — matches your original
   `<BleKeyboard.h>` include, fetched automatically on first build.

## Build / upload

```bash
pio run                 # build
pio run -t upload       # flash
pio device monitor      # serial monitor (115200 baud)
```

Or with the VS Code PlatformIO extension: open this folder, use the
PlatformIO sidebar's Build/Upload/Monitor buttons.
