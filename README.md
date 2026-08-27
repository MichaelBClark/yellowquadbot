# yellowquadbot

A 4-legged (quadruped) walking robot, 3 servos per leg (coxa / femur / tibia
— 12 servos total), plus a wireless joystick controller.

## Hardware

**Robot body**
- ESP32 WROOM (DevKit-style board) — main controller
- HiLetgo PCA9685 16-channel 12-bit PWM servo driver (I2C) — drives all 12 leg servos
- 12x hobby servos (3 per leg x 4 legs)
- External 5-6V BEC/UBEC for servo power (do **not** power servos from the ESP32 5V pin)

**Joystick**
- Seeed Studio XIAO ESP32C6
- 2x analog thumb joysticks (X/Y, with press button) — left = move, right = turn/look
- 1-2 extra momentary buttons (deadman / enable, gait mode toggle)
- Optional: small LiPo + charge circuit for a handheld build

**Link:** ESP-NOW (WiFi peer-to-peer, no router/pairing needed, low latency).
The joystick broadcasts its MAC-address-targeted packets to the robot's known
MAC address; swap in the robot's actual MAC in `joystick/src/main.cpp` once
you flash the controller and read its address from the serial log.

## Repo layout

```
controller/   PlatformIO project for the robot body (ESP32 WROOM + PCA9685)
joystick/     PlatformIO project for the handheld controller (XIAO ESP32C6)
docs/         wiring notes, calibration notes
```

Each subdirectory is its own PlatformIO project (different boards), so open
whichever one you're working on, or use `pio run` from inside it.

## Quick start

1. `cd controller && pio run -t upload -t monitor` — flash the robot, note the
   MAC address it prints on boot.
2. Put that MAC address into `joystick/src/main.cpp` (`ROBOT_MAC`).
3. `cd joystick && pio run -t upload` — flash the joystick.
4. Power the robot's servo rail from a separate 5-6V supply, connect PCA9685
   `V+` to it and `VCC` to the ESP32's 3.3V (logic only).
5. Hold the deadman button on the joystick to arm the legs; release it and
   the robot immediately relaxes to a safe standing pose.

See `docs/wiring.md` and `docs/calibration.md` for pinout and per-servo trim
instructions.
