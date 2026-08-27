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

| Signal                    | XIAO C6 pin |
|---------------------------|-------------|
| Left stick X              | A0 (GPIO0)  |
| Left stick Y              | A1 (GPIO1)  |
| Right stick X             | A2 (GPIO2)  |
| Right stick Y             | A3 (GPIO3)  |
| Deadman/enable button     | GPIO4 (to GND, uses internal pullup) |
| Gait-mode toggle button   | GPIO5 (to GND, uses internal pullup) |

Analog joystick modules typically have 5 pins: GND, +5V (or +3.3V), VRx,
VRy, SW (button, if present). Power the joystick modules from the XIAO's
3.3V rail — the C6's ADC expects 0-3.3V input.

See `joystick/include/joystick_config.h` to change any of these pin
assignments.
