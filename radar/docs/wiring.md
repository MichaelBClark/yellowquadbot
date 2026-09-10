# Wiring

## Display / touch

Already wired on-board (QSPI to the AMOLED module, I2C to the touch
controller) — nothing to connect there. You only need to get the pin
*numbers* right in `include/pins.h`, copied from Waveshare's demo code as
described in the top-level README.

## Ultrasonic sensor (HC-SR04)

| HC-SR04 pin | Connects to |
|-------------|-------------|
| VCC         | 5V |
| GND         | GND (common with servo and board) |
| TRIG        | `ULTRASONIC_TRIG` GPIO (direct, 3.3V logic out is fine as a trigger input to the sensor) |
| ECHO        | `ULTRASONIC_ECHO` GPIO **through a voltage divider** |

**Important:** the HC-SR04's ECHO pin outputs a 5V logic signal. The
ESP32-S3's GPIOs are not 5V-tolerant — feeding ECHO straight into a GPIO
risks damaging the pin. Use a simple resistor divider: ECHO through a 1kΩ
resistor to the GPIO, then a 2kΩ resistor from that same GPIO node down to
GND. That divides 5V down to a safe ~3.3V for the ESP32 to read, without
noticeably affecting TRIG (which the ESP32 drives, not receives).

## Servo, via PCA9685

The servo doesn't connect to the ESP32-S3 directly — it goes through a
PCA9685 I2C PWM driver, same part used for the yellowquadbot legs
elsewhere in this repo.

| PCA9685 pin | Connects to |
|-------------|-------------|
| VCC (logic) | 3.3V |
| GND | GND (common with everything else) |
| SDA | `PCA9685_SDA` (defaults to the touch controller's SDA — same bus, different address, see below) |
| SCL | `PCA9685_SCL` (defaults to the touch controller's SCL) |
| V+ (servo power rail) | External 5V supply, **not** the ESP32-S3 board's 5V/USB pin |
| Channel 0 (or whichever you set `SERVO_CHANNEL` to) signal pin | Servo signal wire (orange/white) |

Servo's own + and GND wires go to the PCA9685's screw terminal alongside
V+ and GND, not to the ESP32-S3 board.

**Sharing the I2C bus with touch:** I2C is a shared bus by design — multiple
devices can sit on the same SDA/SCL lines as long as they have different
addresses. The PCA9685 defaults to `0x40` (all address jumpers open),
which won't collide with the touch controller's address, so reusing
`TOUCH_SDA`/`TOUCH_SCL` for the PCA9685 (as `pins.h` does by default)
saves you from needing two more free GPIOs. If that turns out not to work
for your specific board/wiring, wire the PCA9685 to its own free GPIO
pair instead and change `PCA9685_SDA`/`PCA9685_SCL` in `pins.h`
accordingly — either way works, this is just the pin-frugal default.

A single small servo (SG90-class) can usually run off a basic 5V USB
supply into the PCA9685's V+ terminal. If you add more servos later or
upgrade to something higher-torque, budget the supply for their combined
stall current — same reasoning as any multi-servo project.

## Picking free GPIOs (ultrasonic sensor)

The display (QSPI, 4-6 pins), touch controller (I2C + interrupt/reset),
onboard IMU and RTC (I2C), and battery voltage monitor (ADC) all claim
GPIOs on this board already. Check the pinout diagram on Waveshare's wiki
page for this product to see which GPIOs are broken out on the
unpopulated header/pads and not already spoken for, then use any two of
those for `ULTRASONIC_TRIG`/`ULTRASONIC_ECHO` in `include/pins.h`. Neither
needs to be anything special — plain digital I/O is enough.
