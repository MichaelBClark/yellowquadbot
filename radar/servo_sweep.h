#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

// Drives a standard hobby servo through a PCA9685 I2C PWM driver and
// sweeps it back and forth between two angles, like a classic ping-radar
// head. Pass in an already-begin()'d Adafruit_PWMServoDriver.
class ServoSweep {
public:
  ServoSweep(Adafruit_PWMServoDriver &pwm, uint8_t channel, int minDeg = 15,
             int maxDeg = 165, float degPerSec = 60.0f)
      : pwm_(pwm), channel_(channel), minDeg_(minDeg), maxDeg_(maxDeg),
        degPerSec_(degPerSec), angle_(minDeg), direction_(1) {}

  void begin() { writeAngle(angle_); }

  // Call every loop iteration; advances the sweep angle based on elapsed
  // time and returns the current commanded angle in degrees.
  float update() {
    uint32_t now = micros();
    if (lastUpdateUs_ == 0) lastUpdateUs_ = now;
    float dtSec = (now - lastUpdateUs_) / 1000000.0f;
    lastUpdateUs_ = now;

    angle_ += direction_ * degPerSec_ * dtSec;
    if (angle_ >= maxDeg_) {
      angle_ = maxDeg_;
      direction_ = -1;
    } else if (angle_ <= minDeg_) {
      angle_ = minDeg_;
      direction_ = 1;
    }

    writeAngle(angle_);
    return angle_;
  }

  float currentAngle() const { return angle_; }

private:
  void writeAngle(float deg) {
    deg = constrain(deg, 0.0f, 180.0f);
    // Standard hobby servo: 500-2500us pulse. The PCA9685 counts run
    // 0-4095 over a 20ms (50Hz) period, so convert pulse width to ticks.
    uint32_t pulseUs = 500 + (uint32_t)((deg / 180.0f) * 2000.0f);
    uint16_t ticks = (uint16_t)((pulseUs * 4096UL) / 20000UL);
    pwm_.setPWM(channel_, 0, ticks);
  }

  Adafruit_PWMServoDriver &pwm_;
  uint8_t channel_;
  int minDeg_, maxDeg_;
  float degPerSec_;
  float angle_;
  int direction_;
  uint32_t lastUpdateUs_ = 0;
};
