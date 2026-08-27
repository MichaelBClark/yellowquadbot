#pragma once

// XIAO ESP32C6 pin assignments. Adjust to match your wiring.
// The C6's ADC-capable pins: A0-A3 (GPIO0-3). Buttons use any digital pin.
constexpr int PIN_LEFT_X = 0;   // A0 - left stick X (strafe)
constexpr int PIN_LEFT_Y = 1;   // A1 - left stick Y (forward/back)
constexpr int PIN_RIGHT_X = 2;  // A2 - right stick X (turn)
constexpr int PIN_RIGHT_Y = 3;  // A3 - right stick Y (body height trim)

constexpr int PIN_DEADMAN = 4;    // momentary button, active-low, hold to arm
constexpr int PIN_GAIT_TOGGLE = 5; // momentary button, active-low

constexpr int ADC_MAX = 4095; // 12-bit ADC on ESP32C6
constexpr int ADC_CENTER = ADC_MAX / 2;

// Fill in with the value printed by the controller firmware on boot
// ("Robot MAC address: ..."), e.g. {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}.
constexpr uint8_t ROBOT_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
