#pragma once

// ============================================================================
// Pin numbers below are confirmed from Waveshare's own docs for this exact
// board (ESP32-S3-Touch-LCD-1.46, docs.waveshare.com/ESP32-S3-Touch-LCD-1.46).
// ============================================================================

// ---- Display (QSPI, 4 data lines + clock + chip-select) ----
#define LCD_SDIO0   46
#define LCD_SDIO1   45
#define LCD_SDIO2   42
#define LCD_SDIO3   41
#define LCD_SCLK    40
#define LCD_CS      21
#define LCD_TE      18   // tearing-effect pin
#define LCD_BL      5    // backlight, plain GPIO - HIGH to turn on
#define LCD_WIDTH   412
#define LCD_HEIGHT  412

// !! LCD_RST is NOT a plain ESP32 GPIO !!
// Waveshare's pinout lists it as "EXIO2" - wired through an onboard I2C
// GPIO expander, not directly to the ESP32-S3. radar.ino drives it via
// IoExpander (io_expander.h) using the standard PCA9554/TCA9554 register
// layout, found on address IO_EXPANDER_I2C_ADDR below.
#define LCD_RST_EXIO_PIN 2   // expander pin number, NOT an ESP32 GPIO

// TODO: confirm this against radar.ino's boot-time I2C scan output (it
// prints every address found on the bus before touching the display).
// 0x20 is the PCA9554/TCA9554 family's default when all address pins are
// tied low, which is the common default for this role - but "common
// default" isn't "confirmed for your board", so check the scan.
#define IO_EXPANDER_I2C_ADDR 0x20

// ---- Touch controller (I2C) ----
// This is the SAME bus as the board's exposed 2-pin I2C header (GND/3V3/
// SCL/SDA), confirmed by Waveshare's docs listing the header's SCL/SDA on
// the identical GPIO10/GPIO11 as TP_SCL/TP_SDA.
#define TOUCH_SDA   11
#define TOUCH_SCL   10
#define TOUCH_INT   4

// !! TP_RST is ALSO on the I2C expander, not a plain GPIO !!
#define TOUCH_RST_EXIO_PIN 1   // expander pin number, NOT an ESP32 GPIO

// ============================================================================
// Servo (via PCA9685) + ultrasonic sensor pins.
// ============================================================================

// PCA9685 shares the I2C bus with the touch controller / exposed I2C
// header (same SDA/SCL, different address) rather than claiming more GPIOs.
#define PCA9685_SDA       TOUCH_SDA
#define PCA9685_SCL       TOUCH_SCL
#define PCA9685_I2C_ADDR  0x40   // default PCA9685 address (all A0-A5 jumpers open)
#define SERVO_CHANNEL     0      // PCA9685 output channel the servo is wired to

// This board only exposes two other digital pins on a header (besides the
// I2C pair above): the UART TXD/RXD pair, GPIO43/44. Waveshare's docs note
// these can be used as plain GPIO instead of UART - which is what we do
// here, since this sketch's serial console runs over the native USB CDC
// port (ARDUINO_USB_CDC_ON_BOOT), not this UART.
#define ULTRASONIC_TRIG   43
#define ULTRASONIC_ECHO   44   // see docs/wiring.md re: 5V echo signal - needs a voltage divider
