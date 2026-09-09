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

## Servo

| Servo wire | Connects to |
|------------|-------------|
| Signal (orange/white) | `SERVO_PIN` GPIO |
| + (red) | 5V |
| GND (brown/black) | GND (common with sensor and board) |

A single small servo (SG90-class) can usually run off the board's own 5V
rail. If you swap in a larger/higher-torque servo, or add more later,
power it from a separate 5V supply with grounds tied together instead —
same reasoning as any servo project: don't let a stalling servo brown out
the microcontroller.

## Picking free GPIOs

The display (QSPI, 4-6 pins), touch controller (I2C + interrupt/reset),
onboard IMU and RTC (I2C), and battery voltage monitor (ADC) all claim
GPIOs on this board already. Check the pinout diagram on Waveshare's wiki
page for this product to see which GPIOs are broken out on the
unpopulated header/pads and not already spoken for, then use any two of
those for `SERVO_PIN`/`ULTRASONIC_TRIG`/`ULTRASONIC_ECHO` in
`include/pins.h`. Any GPIO works for these three — none of them need to be
ADC-capable or anything special (the servo uses the ESP32's LEDC PWM
peripheral, which can attach to any pin).
