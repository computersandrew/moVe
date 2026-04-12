#include "g_meter.h"

#include "aircraft_math.h"

#include <stddef.h>

#define G_METER_MIN_DISPLAY_G       -5.0f
#define G_METER_MAX_DISPLAY_G       10.0f

void GMeter_Init(GMeterData *g_meter)
{
    if (g_meter == NULL) {
        return;
    }

    g_meter->normal_g = 1.0f;
    g_meter->peak_positive_g = 1.0f;
    g_meter->peak_negative_g = 1.0f;
    g_meter->valid = 0u;
}

void GMeter_Update(GMeterData *g_meter,
                   float normal_accel_g,
                   float dt_s,
                   float smoothing_tau_s)
{
    normal_accel_g = AircraftMath_Clamp(normal_accel_g,
                                        G_METER_MIN_DISPLAY_G,
                                        G_METER_MAX_DISPLAY_G);

    if (g_meter == NULL) {
        return;
    }

    if (g_meter->valid == 0u) {
        g_meter->normal_g = normal_accel_g;
        g_meter->peak_positive_g = normal_accel_g;
        g_meter->peak_negative_g = normal_accel_g;
    } else {
        g_meter->normal_g = AircraftMath_LowPass(g_meter->normal_g,
                                                 normal_accel_g,
                                                 dt_s,
                                                 smoothing_tau_s);
        if (normal_accel_g > g_meter->peak_positive_g) {
            g_meter->peak_positive_g = normal_accel_g;
        }
        if (normal_accel_g < g_meter->peak_negative_g) {
            g_meter->peak_negative_g = normal_accel_g;
        }
    }

    g_meter->valid = 1u;
}

void GMeter_ResetPeaks(GMeterData *g_meter)
{
    if (g_meter == NULL) {
        return;
    }

    g_meter->peak_positive_g = g_meter->normal_g;
    g_meter->peak_negative_g = g_meter->normal_g;
}
