#include "altimeter.h"

#include "aircraft_math.h"

#include <stddef.h>

void Altimeter_Init(AltimeterData *altimeter)
{
    if (altimeter == NULL) {
        return;
    }

    altimeter->altitude_m = 0.0f;
    altimeter->altitude_ft = 0.0f;
    altimeter->display_altitude_ft = 0;
    altimeter->valid = 0u;
}

void Altimeter_Update(AltimeterData *altimeter,
                      float altitude_m,
                      uint8_t baro_valid,
                      float dt_s,
                      float smoothing_tau_s)
{
    const float altitude_ft = altitude_m * AIRCRAFT_M_TO_FT;

    if (altimeter == NULL) {
        return;
    }

    if (baro_valid == 0u) {
        altimeter->valid = 0u;
        return;
    }

    if (altimeter->valid == 0u) {
        altimeter->altitude_m = altitude_m;
        altimeter->altitude_ft = altitude_ft;
    } else {
        altimeter->altitude_m = AircraftMath_LowPass(altimeter->altitude_m,
                                                     altitude_m,
                                                     dt_s,
                                                     smoothing_tau_s);
        altimeter->altitude_ft = AircraftMath_LowPass(altimeter->altitude_ft,
                                                      altitude_ft,
                                                      dt_s,
                                                      smoothing_tau_s);
    }

    altimeter->display_altitude_ft = AircraftMath_AltitudeToDisplayFt(altimeter->altitude_ft);
    altimeter->valid = 1u;
}
