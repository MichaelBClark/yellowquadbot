#pragma once

// Display, touch, backlight, and I2C-expander pins/config are now owned by
// the driver files copied from Waveshare's own working example
// (waveshareteam/ESP32-S3-Touch-LCD-1.46, example/Arduino-3.1.1/examples/
// LVGL_Arduino) - see Display_SPD2010.h, I2C_Driver.h, TCA9554PWR.h. This
// file only covers what's specific to the radar app: the ultrasonic
// sensor and the PCA9685 servo driver.

// ============================================================================
// Servo (via PCA9685) + ultrasonic sensor pins.
// ============================================================================

// PCA9685 shares the I2C bus with the touch controller / IO expander
// (I2C_SDA_PIN/I2C_SCL_PIN in I2C_Driver.h, GPIO11/GPIO10) - same bus,
// different address (PCA9685 default 0x40 vs TCA9554's 0x20 vs the touch
// controller's 0x53), so it doesn't need its own GPIOs.
#define PCA9685_I2C_ADDR  0x40   // default PCA9685 address (all A0-A5 jumpers open)
#define SERVO_CHANNEL     0      // PCA9685 output channel the servo is wired to

// This board only exposes two other digital pins on a header (besides the
// I2C pair used above): the UART TXD/RXD pair, GPIO43/44. Waveshare's docs
// note these can be used as plain GPIO instead of UART - which is what we
// do here, since this sketch's serial console runs over the native USB
// CDC port (ARDUINO_USB_CDC_ON_BOOT), not this UART.
#define ULTRASONIC_TRIG   43
#define ULTRASONIC_ECHO   44   // see docs/wiring.md re: 5V echo signal - needs a voltage divider
