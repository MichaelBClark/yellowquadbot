# Wiring

## Robot body (ESP32 WROOM + PCA9685)

| Signal              | ESP32 WROOM pin |
|---------------------|------------------|
| I2C SDA             | GPIO21           |
| I2C SCL             | GPIO22           |
| PCA9685 VCC (logic) | 3.3V             |
| PCA9685 GND         | GND (common with servo power GND) |
| PCA9685 V+ (servo rail) | External 5-6V BEC/UBEC, **not** the ESP32 5V pin |

PCA9685 channel assignment (see `controller/include/robot_config.h`,
`SERVO_CHANNEL`):

| Leg          | Coxa | Femur | Tibia |
|--------------|------|-------|-------|
| Front-left   | 0    | 1     | 2     |
| Front-right  | 4    | 5     | 6     |
| Rear-left    | 8    | 9     | 10    |
| Rear-right   | 12   | 13    | 14    |

Channels are spaced out (0-2, 4-6, 8-10, 12-14) to make it easy to route each
leg's three servo cables to a contiguous block of the header without
crossing over the neighboring leg's channels. Adjust freely to match your
actual harness — just keep `robot_config.h` in sync.

Power: run servo ground, PCA9685 ground, and ESP32 ground all to one common
ground point. The BEC/UBEC supplying the servos should be rated for the
stall current of all 12 servos if they could all move at once (check your
servo's datasheet; budget ~1A per small hobby servo under load).

## Joystick (XIAO ESP32C6)

Built around an arcade-style joystick — 4 momentary switches inside (up,
down, left, right; pushing the stick in a direction closes that switch) —
plus standalone arcade pushbuttons. Everything here is a digital input, no
ADC involved: each switch/button wires one leg to a GPIO and the other leg
to GND, using the ESP32's internal pullup (no external resistors needed).

| Signal                    | XIAO C6 pin |
|---------------------------|-------------|
| Joystick: up (forward)    | GPIO0       |
| Joystick: down (backward) | GPIO1       |
| Joystick: left (strafe)   | GPIO2       |
| Joystick: right (strafe)  | GPIO3       |
| Deadman/enable button     | GPIO4 (hold to arm) |
| Turn-left button          | GPIO5       |
| Turn-right button         | GPIO6       |
| Gait-mode toggle button   | GPIO7 (tap to cycle) |

Wire each switch/button between its GPIO and GND — nothing else needed. If
your joystick harness brings all 4 direction switches out to a single
5-pin connector (common ground + 4 signal wires), that common wire goes to
GND and the 4 signal wires go to GPIO0-3 in any order — just match up
`joystick_config.h` (`PIN_STICK_UP/DOWN/LEFT/RIGHT`) to however you wired
it, rather than rewiring to match the table.

Diagonal movement (e.g. forward + strafe-left) works automatically since
up/down and left/right are read as independent switches — pushing the
stick to a corner closes two switches at once. There's no turn input on
the joystick itself, so turning in place uses the two dedicated buttons.

See `joystick/include/joystick_config.h` to change any of these pin
assignments, or to add more buttons/switches (e.g. a second gait mode, a
sit/stand toggle) — extend `JoyPacket::buttons` in
`comms_protocol.h` (bits 3-7 are currently unused) and wire the new input
the same way.
