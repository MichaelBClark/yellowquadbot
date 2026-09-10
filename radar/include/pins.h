#pragma once

// ============================================================================
// !! VERIFY THESE AGAINST WAVESHARE'S OWN DEMO CODE BEFORE FLASHING !!
//
// The QSPI display pins below are PLACEHOLDERS. This board's exact GPIO
// wiring is defined by Waveshare's schematic, not something safe to guess.
// Get the real values from the "pin_config.h" (or equivalent) file in the
// demo/example code bundle on this product's Waveshare wiki page:
//   "ESP32-S3 1.46inch Round Display Development Board" -> Resources/Demo
// Copy the LCD_* and TOUCH_* pin numbers from there into this file. Do not
// trust the numbers below as correct for your board.
// ============================================================================

// ---- Display (SH8601 AMOLED, QSPI) ----
#define LCD_SDIO0   -1  // TODO: copy from Waveshare pin_config.h
#define LCD_SDIO1   -1  // TODO
#define LCD_SDIO2   -1  // TODO
#define LCD_SDIO3   -1  // TODO
#define LCD_SCLK    -1  // TODO
#define LCD_CS      -1  // TODO
#define LCD_RST     -1  // TODO (-1 if tied to EN/not separately controlled)
#define LCD_TE      -1  // TODO (tearing-effect pin, if used)
#define LCD_WIDTH   412
#define LCD_HEIGHT  412

// ---- Touch (CST816-family, I2C) ----
// Not used by the radar app, listed here only so you don't accidentally
// reuse these pins for the servo/ultrasonic wiring below.
#define TOUCH_SDA   -1  // TODO
#define TOUCH_SCL   -1  // TODO
#define TOUCH_INT   -1  // TODO
#define TOUCH_RST   -1  // TODO

// ============================================================================
// Servo (via PCA9685) + ultrasonic sensor pins — these ARE yours to choose,
// from whatever GPIOs are broken out on the board's expansion header and
// not already claimed by the display/touch/IMU/RTC/battery-monitor
// circuitry above. Check the board's silkscreen or pinout diagram on the
// wiki, pick free pins, and fill them in here.
// ============================================================================

// PCA9685 shares the I2C bus with the touch controller (same SDA/SCL,
// different address) rather than claiming two more GPIOs. If your board's
// touch bus pins above turn out unusable for a second device for some
// reason, wire the PCA9685 to its own free GPIO pair instead and set
// these independently.
#define PCA9685_SDA       TOUCH_SDA
#define PCA9685_SCL       TOUCH_SCL
#define PCA9685_I2C_ADDR  0x40   // default PCA9685 address (all A0-A5 jumpers open)
#define SERVO_CHANNEL     0      // PCA9685 output channel the servo is wired to

#define ULTRASONIC_TRIG   -1  // TODO: free GPIO, digital output
#define ULTRASONIC_ECHO   -1  // TODO: free GPIO, digital input (see docs/wiring.md re: 5V echo signal)
