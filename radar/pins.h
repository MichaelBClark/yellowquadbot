#pragma once

// Board: ELEGOO EL-EB-009 (ESP32-2432S028R, aka "Cheap Yellow Display"/CYD)
// Display/touch pins are NOT here - TFT_eSPI is configured at the library
// level via a User_Setup.h you copy into the TFT_eSPI library folder, not
// per-sketch. See User_Setup.h in this folder and the README for the copy
// step. This file only covers what's specific to the radar app.
//
// Verified against community reference pinout for this exact board
// (github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md):
// only IO22 and IO27 are genuinely free GPIO (on the CN1 connector) -
// everything else is claimed by the display, touch, SD card, speaker, or
// RGB LED. That's not enough for both an I2C bus (2 pins) and a 2-pin
// ultrasonic sensor, so this reclaims the onboard RGB LED's pins for the
// ultrasonic sensor, per that repo's own suggestion for exactly this
// situation ("if your project requires additional pins... RGB LED might
// be a good candidate to sacrifice").

// ============================================================================
// Servo (via PCA9685) - uses the board's only free GPIO pair (CN1 connector).
// ============================================================================
#define I2C_SDA_PIN       22
#define I2C_SCL_PIN       27
#define PCA9685_I2C_ADDR  0x40   // default PCA9685 address (all A0-A5 jumpers open)
#define SERVO_CHANNEL     0      // PCA9685 output channel the servo is wired to

// ============================================================================
// Ultrasonic sensor - reclaims 2 of the 3 onboard RGB LED pins (Red=IO4,
// Blue=IO17; Green=IO16 left alone, still usable for a status LED if you
// want one). The RGB LED is fully disconnected from anything useful once
// you do this - desolder it or just ignore it.
// ============================================================================
#define ULTRASONIC_TRIG   4
#define ULTRASONIC_ECHO   17   // see docs/wiring.md re: 5V echo signal - needs a voltage divider
