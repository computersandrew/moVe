#include "vertical_speed_indicator.h"

#include "aircraft_math.h"

#include <stddef.h>

#define VSI_MAX_NEEDLE_FPM    4000.0f

void VerticalSpeedIndicator_Init(VerticalSpeedData *vsi)
{
    if (vsi == NULL) {
        return;
    }

    vsi->vertical_speed_mps = 0.0f;
    vsi->vertical_speed_fpm = 0.0f;
    vsi->needle_fpm = 0.0f;
    vsi->valid = 0u;
}

void VerticalSpeedIndicator_Update(VerticalSpeedData *vsi,
                                   float vertical_speed_mps,
                                   uint8_t baro_valid,
                                   float dt_s,
                                   float smoothing_tau_s)
{
    const float vertical_speed_fpm = vertical_speed_mps * AIRCRAFT_MPS_TO_FPM;

    if (vsi == NULL) {
        return;
    }

    if (baro_valid == 0u) {
        vsi->valid = 0u;
        return;
    }

    if (vsi->valid == 0u) {
        vsi->vertical_speed_mps = vertical_speed_mps;
        vsi->vertical_speed_fpm = vertical_speed_fpm;
    } else {
        vsi->vertical_speed_mps = AircraftMath_LowPass(vsi->vertical_speed_mps,
                                                       vertical_speed_mps,
                                                       dt_s,
                                                       smoothing_tau_s);
        vsi->vertical_speed_fpm = AircraftMath_LowPass(vsi->vertical_speed_fpm,
                                                       vertical_speed_fpm,
                                                       dt_s,
                                                       smoothing_tau_s);
    }

    vsi->needle_fpm = AircraftMath_Clamp(vsi->vertical_speed_fpm,
                                         -VSI_MAX_NEEDLE_FPM,
                                         VSI_MAX_NEEDLE_FPM);
    vsi->valid = 1u;
}
