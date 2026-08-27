# Calibration

## 1. Servo pulse range

`SERVO_MIN_TICK`/`SERVO_MAX_TICK` in `controller/include/robot_config.h`
assume a standard 500-2500us hobby servo range mapped onto the PCA9685's
4096-count, 50Hz cycle. If a servo buzzes or hits a hard stop at either end
of travel, narrow the range slightly (e.g. 600-2400us) rather than forcing
it.

## 2. Per-leg trim

With the robot powered and legs disconnected from the body frame (or the
robot up on a stand so feet can move freely):

1. Set all values in `SERVO_TRIM_DEG` and `SERVO_INVERT` to their defaults
   (0, false).
2. Flash, and let the robot settle into its standing pose (deadman not held
   — it drives the standing pose by default).
3. For each leg/joint, observe how far off the physical angle is from the
   intended neutral (coxa pointing straight out, femur/tibia forming the
   `STAND_X/Y/Z` target). Adjust `SERVO_TRIM_DEG[leg][joint]` in degrees to
   correct it.
4. If a joint moves the *wrong direction* relative to the IK's sign
   convention (increasing the commanded angle makes it move down instead of
   up, etc.), set `SERVO_INVERT[leg][joint] = true` for that joint instead
   of trying to fix it with trim alone.
5. Re-flash and repeat until all 4 legs sit symmetrically in the standing
   pose.

## 3. Leg geometry

Measure your actual link lengths (coxa, femur, tibia, in mm) and update
`Geometry::COXA_LEN/FEMUR_LEN/TIBIA_LEN` in `robot_config.h`. Getting these
right matters more than trim for keeping the IK solutions accurate —
mismatched lengths cause the standing pose to look right at one height but
drift as the gait moves the foot around.

## 4. Stride and gait tuning

In `controller/src/main.cpp`, the stick-to-stride mapping:

```
float strideY = (ly / 100.0f) * 35.0f;
float strideX = (lx / 100.0f) * 25.0f + (rx / 100.0f) * 20.0f;
```

controls how many mm the foot sweeps fore/aft and side-to-side at full
stick deflection. Reduce these if the robot tips or legs strike each other;
`Geometry::STEP_HEIGHT` (in `robot_config.h`) controls how high the foot
lifts during swing — increase it if feet drag on rough terrain.
