#include "attitude_indicator.h"

#include "aircraft_math.h"

#include <stddef.h>

#define ATTITUDE_MAX_BANK_DEG     180.0f
#define ATTITUDE_MAX_PITCH_DEG    90.0f

void AttitudeIndicator_Init(AttitudeIndicatorData *attitude)
{
    if (attitude == NULL) {
        return;
    }

    attitude->roll_deg = 0.0f;
    attitude->pitch_deg = 0.0f;
    attitude->bank_pointer_deg = 0.0f;
    attitude->valid = 0u;
}

void AttitudeIndicator_Update(AttitudeIndicatorData *attitude,
                              float roll_deg,
                              float pitch_deg,
                              float dt_s,
                              float smoothing_tau_s)
{
    roll_deg = AircraftMath_Clamp(AircraftMath_Wrap180(roll_deg),
                                  -ATTITUDE_MAX_BANK_DEG,
                                  ATTITUDE_MAX_BANK_DEG);
    pitch_deg = AircraftMath_Clamp(pitch_deg,
                                   -ATTITUDE_MAX_PITCH_DEG,
                                   ATTITUDE_MAX_PITCH_DEG);

    if (attitude == NULL) {
        return;
    }

    if (attitude->valid == 0u) {
        attitude->roll_deg = roll_deg;
        attitude->pitch_deg = pitch_deg;
    } else {
        attitude->roll_deg = AircraftMath_LowPass(attitude->roll_deg,
                                                  roll_deg,
                                                  dt_s,
                                                  smoothing_tau_s);
        attitude->pitch_deg = AircraftMath_LowPass(attitude->pitch_deg,
                                                   pitch_deg,
                                                   dt_s,
                                                   smoothing_tau_s);
    }

    attitude->bank_pointer_deg = attitude->roll_deg;
    attitude->valid = 1u;
}
