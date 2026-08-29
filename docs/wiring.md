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

Built around two proportional RC-gimbal joysticks salvaged from an old RC
unit, each a pair of potentiometers (one per axis) — no protocol, just a
voltage that varies with stick position. Plus two arcade pushbuttons
(deadman, gait toggle).

**Finding each pot's 3 pins** (no datasheet, unbranded stick): with a
multimeter in resistance mode, probe pairs of pins while moving one axis.
The two pins whose resistance sweeps smoothly through its full range
(typically 5-10kΩ) are that axis's **outer legs**; the third pin — whose
resistance to either outer leg changes as the stick moves — is the
**wiper** (signal). Repeat for the other axis. Many gimbals share the
outer-leg rails between both axes, so you may only need to wire
power/ground once per stick (4 wires total: GND, +3.3V, wiper-X, wiper-Y)
rather than 6.

| Signal                    | XIAO C6 pin |
|---------------------------|-------------|
| Left stick X (strafe)     | A0 (GPIO0)  |
| Left stick Y (fwd/back)   | A1 (GPIO1)  |
| Right stick X (turn)      | A2 (GPIO2)  |
| Right stick Y (body height) | A3 (GPIO3) |
| Deadman/enable button     | GPIO4 (hold to arm) |
| Gait-mode toggle button   | GPIO5 (tap to cycle) |

Wiring per axis: outer legs to **3.3V and GND — not 5V**, the C6's ADC
pins aren't 5V-tolerant. It doesn't matter which outer leg goes to which
rail; if an axis reads backwards in software, flip its `INVERT_*` constant
in `joystick_config.h` rather than re-wiring. Wiper → the ADC pin listed
above.

Unbranded RC-surplus pots are rarely mechanically centered, so the
firmware measures each axis's rest voltage at boot (`measureCenter()` in
`main.cpp`) instead of assuming the ADC midpoint — **don't touch the
sticks while the board is booting**, or that calibration will be off. If a
stick still feels off-center after that, check `Serial.printf` output at
boot (prints the measured center for each axis) to sanity-check the pots
are wired to the ADC pins you expect.

See `joystick/include/joystick_config.h` to change any of these pin
assignments. Only 4 ADC1-capable pins are broken out on the XIAO C6 board
(A0-A3), so both sticks' 4 axes use all of them — there's no ADC pin left
for a third stick without moving the deadman/gait buttons off of digital
GPIOs they already use (they don't need ADC pins, any GPIO works).
