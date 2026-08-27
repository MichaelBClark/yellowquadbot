#pragma once
#include <cstdint>

// ESP-NOW packet sent by the joystick to the robot, ~50Hz.
// Keep this struct identical to joystick/include/comms_protocol.h.
struct JoyPacket {
  uint8_t seq;      // rolling sequence number, used to detect stale/lost packets
  int8_t lx, ly;    // left stick: -100..100, walk direction/speed (ly=fwd/back, lx=strafe)
  int8_t rx, ry;     // right stick: -100..100, rx=turn rate, ry=body height trim
  uint8_t buttons;   // bit0: deadman/enable, bit1: gait mode toggle, bit2: aux/trim button
};

// Robot -> joystick status, sent back so the joystick can show battery/state.
struct StatusPacket {
  uint8_t armed;      // 1 if legs are actively being driven
  uint8_t gaitMode;   // current gait index
  float batteryVolts; // 0 if no voltage sense wired up
};
