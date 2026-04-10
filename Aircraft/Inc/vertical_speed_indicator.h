#ifndef VERTICAL_SPEED_INDICATOR_H
#define VERTICAL_SPEED_INDICATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float vertical_speed_mps;
    float vertical_speed_fpm;
    float needle_fpm;
    uint8_t valid;
} VerticalSpeedData;

void VerticalSpeedIndicator_Init(VerticalSpeedData *vsi);
void VerticalSpeedIndicator_Update(VerticalSpeedData *vsi,
                                   float vertical_speed_mps,
                                   uint8_t baro_valid,
                                   float dt_s,
                                   float smoothing_tau_s);

#ifdef __cplusplus
}
#endif

#endif
