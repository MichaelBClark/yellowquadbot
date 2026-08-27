#pragma once
#include <cstdint>

// ---- Leg geometry (mm). Measure your own robot and update these. ----
namespace Geometry {
constexpr float COXA_LEN  = 30.0f;  // hip yaw link
constexpr float FEMUR_LEN = 55.0f;  // upper leg
constexpr float TIBIA_LEN = 80.0f;  // lower leg

// Default standing foot position relative to each leg's coxa origin (mm).
constexpr float STAND_X = 70.0f;  // outward from body
constexpr float STAND_Y = 0.0f;   // fore/aft, 0 = neutral
constexpr float STAND_Z = -60.0f; // down from coxa pivot (negative = down)

constexpr float STEP_HEIGHT = 25.0f; // how high the foot lifts during swing
}

// ---- PCA9685 servo pulse calibration ----
// Typical hobby servo: 500us..2500us maps to a PCA9685 "tick" count at 50Hz.
// PCA9685 counts run 0..4095 over a 20ms (50Hz) period.
constexpr float PCA_FREQ_HZ = 50.0f;
constexpr uint16_t SERVO_MIN_US = 500;
constexpr uint16_t SERVO_MAX_US = 2500;
constexpr uint16_t SERVO_MIN_TICK = 102;  // ~500us at 50Hz/4096
constexpr uint16_t SERVO_MAX_TICK = 490;  // ~2500us at 50Hz/4096

enum LegId { FRONT_LEFT = 0, FRONT_RIGHT = 1, REAR_LEFT = 2, REAR_RIGHT = 3, NUM_LEGS = 4 };
enum JointId { COXA = 0, FEMUR = 1, TIBIA = 2, NUM_JOINTS = 3 };

// PCA9685 channel for each (leg, joint). Update to match your wiring.
// Layout: channels 0-2 = FL, 4-6 = FR, 8-10 = RL, 12-14 = RR (leaves room to
// spread wiring across the board; unused channels stay free for expansion).
constexpr uint8_t SERVO_CHANNEL[NUM_LEGS][NUM_JOINTS] = {
    {0, 1, 2},    // FRONT_LEFT
    {4, 5, 6},    // FRONT_RIGHT
    {8, 9, 10},   // REAR_LEFT
    {12, 13, 14}, // REAR_RIGHT
};

// Per-servo trim, in degrees, added after IK to correct for horn mounting
// offset. Positive/negative per your own calibration pass.
constexpr float SERVO_TRIM_DEG[NUM_LEGS][NUM_JOINTS] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

// Set true for any joint whose servo is mounted mirrored on the physical
// robot, so a positive IK angle needs to be driven as a negative pulse.
constexpr bool SERVO_INVERT[NUM_LEGS][NUM_JOINTS] = {
    {false, false, false},
    {true, false, false},
    {false, false, false},
    {true, false, false},
};

// I2C pins for the PCA9685 (ESP32 WROOM default I2C0).
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;

// Safety: if no valid joystick packet arrives within this window, relax to
// the safe standing pose and stop driving servos aggressively.
constexpr uint32_t PACKET_TIMEOUT_MS = 400;
