# radar

Ultrasonic ping-radar display on a Waveshare ESP32-S3 1.46" round AMOLED
(412x412), servo-swept HC-SR04, unrelated to the yellowquadbot legs/gait
code elsewhere in this repo — just sharing the repo for convenience.

## Hardware

- Waveshare ESP32-S3 1.46" Round Display Development Board (SH8601 AMOLED,
  QSPI, 412x412, CST816-family touch — touch isn't used here)
- HC-SR04 ultrasonic distance sensor
- 1x hobby servo (SG90 or similar) to sweep the sensor
- PCA9685 16-channel I2C PWM/servo driver (the servo is driven through
  this rather than a direct GPIO, sharing the same part used in the
  yellowquadbot leg controller elsewhere in this repo)
- External 5V supply for the servo, wired to the PCA9685's V+ rail — don't
  power it from the ESP32-S3 board's 5V/USB pin

## Before you flash anything

`include/pins.h` has the display/touch pins set to `-1` placeholders.
**Do not guess these.** Get the real numbers from Waveshare's own demo
code for this exact product (download it from the product's page on
their wiki — look for a `pin_config.h` or similar in the demo bundle) and
copy them in. Getting these wrong won't just fail to work, it can drive
pins in ways the AMOLED module doesn't expect.

The ultrasonic sensor's pins are also `-1` placeholders — those you *do*
choose yourself, from whichever GPIOs the board's expansion header breaks
out that aren't already claimed by the display/touch/IMU/RTC/battery
circuitry. The servo doesn't need a GPIO of its own: it's driven through a
PCA9685 sharing the touch controller's I2C bus (different address). See
`docs/wiring.md`.

## How it works

- `ServoSweep` (include/servo_sweep.h) drives the servo through a PCA9685
  I2C PWM driver, sweeping back and forth between two angles.
- `Ultrasonic` (include/ultrasonic.h) pings the HC-SR04 and converts echo
  time to distance.
- `main.cpp` polls both every loop, plots each hit as a fading red blip at
  (servo angle, distance) in polar coordinates on the top semicircle of
  the screen — the classic "ping radar" look — with a green sweep line
  following the servo in real time.

## Tuning

- `ServoSweep(pin, minDeg, maxDeg, degPerSec)` in `main.cpp` — narrower
  sweep range or slower `degPerSec` gives the ultrasonic sensor more time
  per angle, which matters since HC-SR04 pings take a few ms and the code
  waits for one before moving on.
- `MAX_RANGE_CM` / `Ultrasonic(...)`'s max range argument — set to
  whatever your sensor and use case need; readings beyond it are dropped
  rather than plotted, so the display doesn't get confusing false-far
  blips.
- `FADE_FRAMES` — how many frames a blip stays visible before fading out.

## A performance note

Every frame does a full-screen redraw (clear + grid + blips + sweep line)
over QSPI, which is simple but not the fastest way to drive a 412x412
panel — expect it to look more like a slow, deliberate radar sweep than a
smooth 60fps animation. That fits a radar display reasonably well as-is;
if you want it snappier later, the straightforward next step is switching
to partial-region redraws (only clear/redraw the sweep line's old and new
position, plus blips as they're added/faded) instead of clearing the
whole screen every loop.
