#pragma once
#include <math.h>
#include "robot_config.h"

// Trot gait: diagonal leg pairs (FL+RR, FR+RL) swing together, the other
// pair stays planted. This is the simplest stable gait for a quadruped and
// is enough for a joystick-controlled walker.
//
// phase is 0..1, advanced every update() call based on walk speed.
// Each leg's own phase offset is 0 for one diagonal pair, 0.5 for the other.
namespace Gait {

constexpr float LEG_PHASE_OFFSET[NUM_LEGS] = {
    0.0f,  // FRONT_LEFT  (pair A)
    0.5f,  // FRONT_RIGHT (pair B)
    0.5f,  // REAR_LEFT   (pair B)
    0.0f,  // REAR_RIGHT  (pair A)
};

// Computes the foot offset (dx, dy, dz) added on top of the standing pose
// for one leg at the given global gait phase and per-leg stride (forward,
// strafe) amount.
struct FootOffset { float dx, dy, dz; };

inline FootOffset legOffset(int legId, float globalPhase, float strideY, float strideX) {
  float phase = fmodf(globalPhase + LEG_PHASE_OFFSET[legId], 1.0f);

  FootOffset out{0, 0, 0};
  if (phase < 0.5f) {
    // Swing phase: foot lifts and moves forward through the air.
    float t = phase / 0.5f;              // 0..1 across the swing
    float s = sinf(t * (float)M_PI);      // 0..1..0 lift profile
    out.dz = Geometry::STEP_HEIGHT * s;
    out.dy = strideY * (t * 2.0f - 1.0f); // sweeps from -stride to +stride
    out.dx = strideX * (t * 2.0f - 1.0f);
  } else {
    // Stance phase: foot stays on the ground, pushes body forward.
    float t = (phase - 0.5f) / 0.5f;      // 0..1 across stance
    out.dz = 0.0f;
    out.dy = strideY * (1.0f - t * 2.0f); // sweeps from +stride to -stride
    out.dx = strideX * (1.0f - t * 2.0f);
  }
  return out;
}

} // namespace Gait
