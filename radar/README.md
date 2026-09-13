# radar

Ultrasonic ping-radar display on an ELEGOO EL-EB-009 (ESP32-2432S028R,
the well-known "Cheap Yellow Display"/CYD — ELEGOO's branded version of
it), servo-swept HC-SR04. Unrelated to the yellowquadbot legs/gait code
elsewhere in this repo — just sharing the repo for convenience. This is
step one of a larger plan: eventually this same ESP32 will drive drive
motors too, so the robot can move around and avoid obstacles using the
same ultrasonic sensor.

## Hardware

- ELEGOO EL-EB-009 / ESP32-2432S028R (320x240 ILI9341 TFT + XPT2046
  resistive touch, ESP32-WROOM-32, USB-C) — pinout verified against
  github.com/witnessmenow/ESP32-Cheap-Yellow-Display
- HC-SR04 ultrasonic distance sensor
- 1x hobby servo (SG90 or similar) to sweep the sensor
- PCA9685 16-channel I2C PWM/servo driver (the servo is driven through
  this rather than a direct GPIO, sharing the same part used in the
  yellowquadbot leg controller elsewhere in this repo)
- External 5V supply for the servo, wired to the PCA9685's V+ rail — don't
  power it from the board's own 5V/USB pin

## This board is nearly out of GPIO

Only **IO22 and IO27** are genuinely free on this board — everything else
is claimed by the display, touch controller, SD card slot, speaker, or
onboard RGB LED. That's exactly enough for one I2C bus (used here for the
PCA9685), with nothing left over for the ultrasonic sensor's two pins.
`pins.h` reclaims 2 of the 3 onboard RGB LED pins for that — the board's
own docs suggest this as the go-to move when you need more pins than the
official ones ("if your project requires additional pins... RGB LED might
be a good candidate to sacrifice"). The RGB LED becomes non-functional
once you do this; safe to ignore or desolder.

**Planning ahead for the "move around and avoid obstacles" phase**: there
are no GPIOs left on this board for drive motors. A typical motor driver
(L298N, TB6612, etc.) needs 2-4 more digital pins per motor. When you get
there, the options are: an I2C-based motor driver riding the same bus as
the PCA9685 (keeps pin count to zero extra), or moving the "brain" to a
second, GPIO-richer microcontroller that talks to this board over serial/
I2C/ESP-NOW for the display only. Worth deciding before wiring motors in,
not after.

## Building with the Arduino IDE

This is a standard Arduino sketch — open `radar.ino` (the folder is named
`radar` to match, as Arduino requires) directly in the Arduino IDE.

1. **Board support**: `File → Preferences → Additional Boards Manager URLs`,
   add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   if not already present, then `Tools → Board → Boards Manager`, search
   "esp32", install the Espressif package (a recent 3.x release).
2. **Board selection**: `Tools → Board → esp32 → ESP32 Dev Module` (this
   board uses a plain ESP32-WROOM-32, not an S3/S2/C-series).
3. **Libraries**: `Sketch → Include Library → Manage Libraries`, install:
   - **TFT_eSPI** (by Bodmer) — display driver
   - **Adafruit PWM Servo Driver Library** — PCA9685
4. **Configure TFT_eSPI** — this library is configured at the library
   level, not per-sketch:
   - Find where the library manager installed TFT_eSPI (typically
     `Documents/Arduino/libraries/TFT_eSPI/` on Windows/Mac, or the
     `libraries/` folder wherever your Arduino IDE sketchbook lives).
   - **Copy `User_Setup.h` from this sketch folder, overwriting**
     `TFT_eSPI/User_Setup.h`. Don't just add it alongside — it must
     replace the library's own default, which is for a different board
     entirely and will produce garbled/no display otherwise (same
     failure mode as the wrong display driver did on the previous
     board this project used).
5. Select the right serial port and upload.

## Before you flash anything

`pins.h` has the I2C (PCA9685) and ultrasonic pins already filled in from
the verified pinout above — nothing to guess there. Just make sure
`User_Setup.h` actually made it into the TFT_eSPI library folder (step 4
above) before flashing, or the display init will silently use the wrong
pins.

## How it works

- `ServoSweep` (servo_sweep.h) drives the servo through a PCA9685
  I2C PWM driver, sweeping back and forth between two angles.
- `Ultrasonic` (ultrasonic.h) pings the HC-SR04 and converts echo
  time to distance.
- `radar.ino` polls both every loop, plots each hit as a fading red blip at
  (servo angle, distance) in polar coordinates on the top semicircle of
  the screen — the classic "ping radar" look — with a green sweep line
  following the servo in real time. Drawn directly via TFT_eSPI's
  primitives (`fillScreen`/`drawLine`/`fillCircle`) — no manual
  framebuffer needed here, unlike the previous board's SPD2010 driver
  which only exposed a raw rectangular blit.

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
- `tft.setRotation(1)` in `setup()` — try `3` instead if the display
  comes up upside-down relative to how you've mounted the board.
