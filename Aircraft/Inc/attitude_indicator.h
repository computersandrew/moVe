#ifndef ATTITUDE_INDICATOR_H
#define ATTITUDE_INDICATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float bank_pointer_deg;
    uint8_t valid;
} AttitudeIndicatorData;

void AttitudeIndicator_Init(AttitudeIndicatorData *attitude);
void AttitudeIndicator_Update(AttitudeIndicatorData *attitude,
                              float roll_deg,
                              float pitch_deg,
                              float dt_s,
                              float smoothing_tau_s);

#ifdef __cplusplus
}
#endif

#endif
