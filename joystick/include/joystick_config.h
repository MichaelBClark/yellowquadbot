#pragma once

// XIAO ESP32C6 pin assignments. Adjust to match your wiring.
//
// Two proportional (potentiometer) RC-gimbal-style joysticks. Each axis is
// a simple pot wired as a voltage divider (outer legs to 3.3V/GND, wiper to
// an ADC pin) — no protocol, just a variable voltage. The C6 exposes 4
// ADC1-capable pins as A0-A3, exactly enough for both sticks' X/Y.
constexpr int PIN_LEFT_X = 0;   // A0 - left stick X (strafe)
constexpr int PIN_LEFT_Y = 1;   // A1 - left stick Y (forward/back)
constexpr int PIN_RIGHT_X = 2;  // A2 - right stick X (turn)
constexpr int PIN_RIGHT_Y = 3;  // A3 - right stick Y (body height trim)

constexpr int PIN_DEADMAN = 4;      // momentary button, active-low, hold to arm
constexpr int PIN_GAIT_TOGGLE = 5;  // momentary button, active-low, tap to cycle gait

constexpr int ADC_MAX = 4095; // 12-bit ADC on ESP32C6

// Unbranded RC-surplus pots are rarely dead-centered mechanically, so the
// firmware measures each axis's resting voltage at boot instead of
// assuming ADC_MAX/2. Set true for any axis that reads backwards on your
// unit (moving the stick "up" decreases the reading) rather than
// re-wiring it.
constexpr bool INVERT_LEFT_X = false;
constexpr bool INVERT_LEFT_Y = true;   // stick "up" -> positive forward
constexpr bool INVERT_RIGHT_X = false;
constexpr bool INVERT_RIGHT_Y = true;

// Fill in with the value printed by the controller firmware on boot
// ("Robot MAC address: ..."), e.g. {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}.
constexpr uint8_t ROBOT_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
