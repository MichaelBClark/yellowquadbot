#pragma once
#include <Wire.h>

// Minimal driver for the standard PCA9554/TCA9554-style 8-bit I2C GPIO
// expander, which is what "EXIO" pins on Waveshare's boards are almost
// always wired through. This is the near-universal register layout for
// that whole chip family, regardless of the exact part number silkscreened
// on your board:
//   0x00 input port (read-only)
//   0x01 output port
//   0x02 polarity inversion (unused here)
//   0x03 configuration (1 = input, 0 = output, per bit)
class IoExpander {
public:
  IoExpander(TwoWire &wire, uint8_t addr) : wire_(wire), addr_(addr) {}

  void pinModeOutput(uint8_t pin) {
    uint8_t cfg = readReg(REG_CONFIG);
    cfg &= ~(1 << pin); // 0 = output
    writeReg(REG_CONFIG, cfg);
  }

  void digitalWrite(uint8_t pin, bool level) {
    uint8_t out = readReg(REG_OUTPUT);
    if (level) out |= (1 << pin);
    else out &= ~(1 << pin);
    writeReg(REG_OUTPUT, out);
  }

  // Pulses a pin low then high, holding each state for holdMs - the usual
  // "reset" sequence for an active-low RST line. Call pinModeOutput() on
  // the pin first.
  void pulseResetLow(uint8_t pin, uint32_t holdMs = 10) {
    digitalWrite(pin, false);
    delay(holdMs);
    digitalWrite(pin, true);
    delay(holdMs);
  }

private:
  static constexpr uint8_t REG_OUTPUT = 0x01;
  static constexpr uint8_t REG_CONFIG = 0x03;

  uint8_t readReg(uint8_t reg) {
    wire_.beginTransmission(addr_);
    wire_.write(reg);
    wire_.endTransmission(false);
    wire_.requestFrom((int)addr_, 1);
    if (wire_.available()) return wire_.read();
    return 0xFF; // all-input default if the read failed
  }

  void writeReg(uint8_t reg, uint8_t value) {
    wire_.beginTransmission(addr_);
    wire_.write(reg);
    wire_.write(value);
    wire_.endTransmission();
  }

  TwoWire &wire_;
  uint8_t addr_;
};
