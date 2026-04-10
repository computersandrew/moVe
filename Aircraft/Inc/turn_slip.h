#ifndef TURN_SLIP_H
#define TURN_SLIP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AIRCRAFT_STANDARD_RATE_TURN_DPS    3.0f

typedef struct
{
    float turn_rate_deg_s;
    float standard_rate_fraction;
    float slip_ball;
    uint8_t valid;
} TurnSlipData;

void TurnSlip_Init(TurnSlipData *turn_slip);
void TurnSlip_Update(TurnSlipData *turn_slip,
                     float yaw_rate_dps,
                     float lateral_accel_g,
                     float dt_s,
                     float smoothing_tau_s);

#ifdef __cplusplus
}
#endif

#endif
