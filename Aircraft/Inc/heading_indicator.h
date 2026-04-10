#ifndef HEADING_INDICATOR_H
#define HEADING_INDICATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float magnetic_heading_deg;
    float true_heading_deg;
    uint16_t display_heading_deg;
    uint8_t magnetic_valid;
} HeadingData;

void HeadingIndicator_Init(HeadingData *heading,
                           float magnetic_declination_deg);

void HeadingIndicator_Update(HeadingData *heading,
                             float yaw_deg,
                             uint8_t magnetic_valid,
                             float magnetic_declination_deg,
                             float dt_s,
                             float smoothing_tau_s);

#ifdef __cplusplus
}
#endif

#endif
