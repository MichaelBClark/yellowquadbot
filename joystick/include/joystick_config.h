#pragma once

// XIAO ESP32C6 pin assignments. Adjust to match your wiring.
//
// Arcade joystick: 4 momentary switches (up/down/left/right), each closes
// to GND when pushed in that direction. All inputs here are digital,
// active-low, using internal pullups — no ADC involved.
constexpr int PIN_STICK_UP = 0;    // forward
constexpr int PIN_STICK_DOWN = 1;  // backward
constexpr int PIN_STICK_LEFT = 2;  // strafe left
constexpr int PIN_STICK_RIGHT = 3; // strafe right

constexpr int PIN_DEADMAN = 4;      // hold to arm the legs
constexpr int PIN_TURN_LEFT = 5;    // rotate in place / turn left
constexpr int PIN_TURN_RIGHT = 6;   // rotate in place / turn right
constexpr int PIN_GAIT_TOGGLE = 7;  // tap to cycle gait mode

// Fill in with the value printed by the controller firmware on boot
// ("Robot MAC address: ..."), e.g. {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}.
constexpr uint8_t ROBOT_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
