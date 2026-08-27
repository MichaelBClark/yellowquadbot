#pragma once
#include <math.h>
#include "robot_config.h"

// Simple 3-DOF (coxa/femur/tibia) leg IK, standard hexapod/quad convention:
//   coxa   rotates in the horizontal plane (yaw) about the leg's mount point
//   femur  pitches the upper leg up/down
//   tibia  pitches the lower leg relative to the femur
//
// Input (x, y, z) is the target foot position in the leg's local frame:
//   x = outward along the coxa's zero-yaw direction
//   y = forward/back
//   z = down (negative = foot below the coxa pivot)
//
// Output angles are in degrees, 0 = neutral/center position of each servo.
struct LegAngles {
  float coxa, femur, tibia;
  bool valid;
};

inline LegAngles solveLegIK(float x, float y, float z) {
  LegAngles out{0, 0, 0, false};

  // Coxa yaw: angle in the horizontal plane to point at the target.
  float coxaAngle = atan2f(y, x);

  // Horizontal distance from the coxa pivot to the target, then subtract
  // the coxa link length to get the femur's horizontal reach.
  float horizDist = sqrtf(x * x + y * y) - Geometry::COXA_LEN;
  float dist = sqrtf(horizDist * horizDist + z * z);

  // Reachability check.
  float maxReach = Geometry::FEMUR_LEN + Geometry::TIBIA_LEN;
  float minReach = fabsf(Geometry::FEMUR_LEN - Geometry::TIBIA_LEN);
  if (dist > maxReach || dist < minReach || dist <= 0.0f) {
    return out; // valid = false, caller should hold last-good pose
  }

  // Law of cosines for femur and tibia angles.
  float a1 = atan2f(z, horizDist);
  float a2 = acosf((Geometry::FEMUR_LEN * Geometry::FEMUR_LEN + dist * dist -
                     Geometry::TIBIA_LEN * Geometry::TIBIA_LEN) /
                    (2.0f * Geometry::FEMUR_LEN * dist));
  float femurAngle = a1 + a2;

  float a3 = acosf((Geometry::FEMUR_LEN * Geometry::FEMUR_LEN +
                     Geometry::TIBIA_LEN * Geometry::TIBIA_LEN - dist * dist) /
                    (2.0f * Geometry::FEMUR_LEN * Geometry::TIBIA_LEN));
  float tibiaAngle = a3 - (float)M_PI / 2.0f;

  out.coxa = coxaAngle * 180.0f / (float)M_PI;
  out.femur = femurAngle * 180.0f / (float)M_PI;
  out.tibia = tibiaAngle * 180.0f / (float)M_PI;
  out.valid = true;
  return out;
}
