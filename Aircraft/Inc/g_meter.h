#ifndef G_METER_H
#define G_METER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float normal_g;
    float peak_positive_g;
    float peak_negative_g;
    uint8_t valid;
} GMeterData;

void GMeter_Init(GMeterData *g_meter);
void GMeter_Update(GMeterData *g_meter,
                   float normal_accel_g,
                   float dt_s,
                   float smoothing_tau_s);
void GMeter_ResetPeaks(GMeterData *g_meter);

#ifdef __cplusplus
}
#endif

#endif
