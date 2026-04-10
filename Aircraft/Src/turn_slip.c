#include "turn_slip.h"

#include "aircraft_math.h"

#include <stddef.h>

#define TURN_SLIP_MAX_TURN_DPS     12.0f
#define TURN_SLIP_MAX_BALL         1.0f

void TurnSlip_Init(TurnSlipData *turn_slip)
{
    if (turn_slip == NULL) {
        return;
    }

    turn_slip->turn_rate_deg_s = 0.0f;
    turn_slip->standard_rate_fraction = 0.0f;
    turn_slip->slip_ball = 0.0f;
    turn_slip->valid = 0u;
}

void TurnSlip_Update(TurnSlipData *turn_slip,
                     float yaw_rate_dps,
                     float lateral_accel_g,
                     float dt_s,
                     float smoothing_tau_s)
{
    const float turn_rate = AircraftMath_Clamp(yaw_rate_dps,
                                               -TURN_SLIP_MAX_TURN_DPS,
                                               TURN_SLIP_MAX_TURN_DPS);
    const float slip_ball = AircraftMath_Clamp(lateral_accel_g,
                                               -TURN_SLIP_MAX_BALL,
                                               TURN_SLIP_MAX_BALL);

    if (turn_slip == NULL) {
        return;
    }

    if (turn_slip->valid == 0u) {
        turn_slip->turn_rate_deg_s = turn_rate;
        turn_slip->slip_ball = slip_ball;
    } else {
        turn_slip->turn_rate_deg_s = AircraftMath_LowPass(turn_slip->turn_rate_deg_s,
                                                          turn_rate,
                                                          dt_s,
                                                          smoothing_tau_s);
        turn_slip->slip_ball = AircraftMath_LowPass(turn_slip->slip_ball,
                                                    slip_ball,
                                                    dt_s,
                                                    smoothing_tau_s);
    }

    turn_slip->standard_rate_fraction =
        AircraftMath_Clamp(turn_slip->turn_rate_deg_s / AIRCRAFT_STANDARD_RATE_TURN_DPS,
                           -1.0f,
                           1.0f);
    turn_slip->valid = 1u;
}
