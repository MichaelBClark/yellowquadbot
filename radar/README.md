# radar

Ultrasonic ping-radar display on a Waveshare ESP32-S3-Touch-LCD-1.46
(412x412 round display), servo-swept HC-SR04, unrelated to the
yellowquadbot legs/gait code elsewhere in this repo — just sharing the
repo for convenience.

## Hardware

- Waveshare ESP32-S3-Touch-LCD-1.46 (412x412 round display, QSPI, SPD2010
  controller; touch isn't used here —
  docs.waveshare.com/ESP32-S3-Touch-LCD-1.46,
  github.com/waveshareteam/ESP32-S3-Touch-LCD-1.46)
- HC-SR04 ultrasonic distance sensor
- 1x hobby servo (SG90 or similar) to sweep the sensor
- PCA9685 16-channel I2C PWM/servo driver (the servo is driven through
  this rather than a direct GPIO, sharing the same part used in the
  yellowquadbot leg controller elsewhere in this repo)
- External 5V supply for the servo, wired to the PCA9685's V+ rail — don't
  power it from the ESP32-S3 board's 5V/USB pin

## Display driver: why this isn't Arduino_GFX

This board's controller is an **SPD2010**, which the popular `GFX Library
for Arduino` doesn't support — its QSPI display classes assume an 8-bit
command width, and SPD2010 needs a 32-bit command width plus a dedicated
vendor driver through ESP-IDF's `esp_lcd_panel` APIs. Using
`Arduino_SH8601` against this panel (an earlier iteration of this file)
compiled and ran without error but produced garbled colored bars — not a
crash, just wrong output, because the init command sequence didn't match
the chip.

Rather than write a from-scratch SPD2010 driver, the following files are
copied verbatim from Waveshare's own working example for this exact board
(`waveshareteam/ESP32-S3-Touch-LCD-1.46`,
`example/Arduino-3.1.1/examples/LVGL_Arduino`):

- `I2C_Driver.h`/`.cpp` — I2C bus setup
- `TCA9554PWR.h`/`.cpp` — the onboard I2C GPIO expander (drives
  `LCD_RST`/`TP_RST`, which aren't plain ESP32 pins)
- `Touch_SPD2010.h`/`.cpp` — touch controller driver (same chip as the
  display; not used by this app, but `LCD_Init()` initializes it as a
  side effect, so it's included to keep that call working as-is rather
  than surgically removing it)
- `esp_lcd_spd2010.h`/`.c` — the actual SPD2010 panel driver
- `Display_SPD2010.h`/`.cpp` — QSPI bus bring-up + `LCD_Init()`/
  `LCD_addWindow()`, the two functions `radar.ino` calls

`radar.ino` keeps its own tiny software framebuffer (a
`LCD_WIDTH * LCD_HEIGHT` array of RGB565 pixels in PSRAM) and draws into
it with plain Bresenham line / midpoint circle routines, since
`LCD_addWindow()` only blits a rectangular pixel buffer — there's no
`drawLine`/`fillCircle` primitive API like `Arduino_GFX` had.

## Building with the Arduino IDE

This is a standard Arduino sketch — open `radar.ino` (the folder is named
`radar` to match, as Arduino requires) directly in the Arduino IDE. The
`.h`/`.cpp` files above show up as additional tabs alongside `radar.ino`.

1. **Board support**: `File → Preferences → Additional Boards Manager URLs`,
   add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   if not already present, then `Tools → Board → Boards Manager`, search
   "esp32", install the Espressif package (a recent 3.x release).
2. **Board selection**: `Tools → Board → esp32 → ESP32S3 Dev Module`. Under
   `Tools`, also set: USB CDC On Boot = Enabled (needed for Serial over
   the native USB port), **PSRAM = OPI PSRAM** (required — the
   framebuffer is allocated in PSRAM via `ps_malloc()` and fails loudly
   over serial if PSRAM isn't enabled).
3. **Libraries**: `Sketch → Include Library → Manage Libraries`, install:
   - **Adafruit PWM Servo Driver Library** — PCA9685
   (no display library needed — the SPD2010 driver files above don't
   depend on one)
4. Select the right serial port and upload.

## Before you flash anything

The ultrasonic sensor's `TRIG`/`ECHO` pins (`pins.h`) are set to this
board's only other exposed digital pins (the UART TXD/RXD header,
repurposed as plain GPIO since this sketch's serial console runs over
native USB, not that UART) — see `docs/wiring.md` for the full pinout and
reasoning. Everything else pin-wise (display, touch, IO expander, I2C) is
hardcoded correctly in the copied driver files, sourced from Waveshare's
own working example rather than guessed.

## How it works

- `ServoSweep` (servo_sweep.h) drives the servo through a PCA9685
  I2C PWM driver, sweeping back and forth between two angles.
- `Ultrasonic` (ultrasonic.h) pings the HC-SR04 and converts echo
  time to distance.
- `radar.ino` polls both every loop, plots each hit as a fading red blip at
  (servo angle, distance) in polar coordinates on the top semicircle of
  the screen — the classic "ping radar" look — with a green sweep line
  following the servo in real time, drawn into a software framebuffer and
  blitted to the panel once per frame via `LCD_addWindow()`.

## Tuning

- `ServoSweep(pwm, channel, minDeg, maxDeg, degPerSec)` in `radar.ino` —
  narrower sweep range or slower `degPerSec` gives the ultrasonic sensor
  more time per angle, which matters since HC-SR04 pings take a few ms and
  the code waits for one before moving on.
- `MAX_RANGE_CM` / `Ultrasonic(...)`'s max range argument — set to
  whatever your sensor and use case need; readings beyond it are dropped
  rather than plotted, so the display doesn't get confusing false-far
  blips.
- `FADE_FRAMES` — how many frames a blip stays visible before fading out.
- `Set_Backlight(80)` in `radar.ino`'s `setup()` — 0-100 brightness.
