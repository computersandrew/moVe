#include "airspeed_indicator.h"

#include "aircraft_math.h"

#include <math.h>
#include <stddef.h>

#define AIRSPEED_MPS_TO_KT             1.943844492f
#define AIRSPEED_MAX_DISPLAY_KT        999.0f
#define ISA_SEA_LEVEL_TEMP_K           288.15f
#define ISA_LAPSE_RATE_K_PER_M         0.0065f
#define ISA_DENSITY_EXPONENT           4.255876f
#define STANDARD_SEA_LEVEL_DENSITY     1.225f
#define SPECIFIC_GAS_CONSTANT_AIR      287.05f
#define CELSIUS_TO_KELVIN              273.15f
#define MIN_STATIC_PRESSURE_PA         30000.0f
#define MAX_STATIC_PRESSURE_PA         125000.0f
#define MIN_AIR_TEMP_K                 213.15f
#define MAX_AIR_TEMP_K                 333.15f

static float density_ratio_from_pressure_altitude(float pressure_altitude_m)
{
    float temp_ratio;

    pressure_altitude_m = AircraftMath_Clamp(pressure_altitude_m, -1000.0f, 11000.0f);
    temp_ratio = 1.0f - ((ISA_LAPSE_RATE_K_PER_M * pressure_altitude_m) / ISA_SEA_LEVEL_TEMP_K);
    temp_ratio = AircraftMath_Clamp(temp_ratio, 0.5f, 1.2f);

    return powf(temp_ratio, ISA_DENSITY_EXPONENT);
}

static float density_ratio_from_pressure_temp(float static_pressure_pa,
                                              float outside_air_temp_c)
{
    const float pressure_pa = AircraftMath_Clamp(static_pressure_pa,
                                                 MIN_STATIC_PRESSURE_PA,
                                                 MAX_STATIC_PRESSURE_PA);
    const float temp_k = AircraftMath_Clamp(outside_air_temp_c + CELSIUS_TO_KELVIN,
                                            MIN_AIR_TEMP_K,
                                            MAX_AIR_TEMP_K);
    const float density = pressure_pa / (SPECIFIC_GAS_CONSTANT_AIR * temp_k);

    return AircraftMath_Clamp(density / STANDARD_SEA_LEVEL_DENSITY, 0.2f, 1.3f);
}

static uint16_t airspeed_to_display_kt(float airspeed_kt)
{
    airspeed_kt = AircraftMath_Clamp(airspeed_kt, 0.0f, AIRSPEED_MAX_DISPLAY_KT);

    return (uint16_t)(airspeed_kt + 0.5f);
}

void AirspeedIndicator_Init(AirspeedData *airspeed)
{
    if (airspeed == NULL) {
        return;
    }

    airspeed->ground_speed_mps = 0.0f;
    airspeed->true_airspeed_mps = 0.0f;
    airspeed->true_airspeed_kt = 0.0f;
    airspeed->display_true_airspeed_kt = 0u;
    airspeed->source = AIRSPEED_SOURCE_NONE;
    airspeed->valid = 0u;
}

void AirspeedIndicator_Update(AirspeedData *airspeed,
                              float gps_ground_speed_mps,
                              uint8_t gps_speed_valid,
                              float pressure_altitude_m,
                              uint8_t pressure_altitude_valid,
                              float static_pressure_pa,
                              float outside_air_temp_c,
                              uint8_t air_density_valid,
                              float dt_s,
                              float smoothing_tau_s)
{
    float target_mps;
    AirspeedSource source;

    if (airspeed == NULL) {
        return;
    }

    if (gps_speed_valid == 0u) {
        airspeed->valid = 0u;
        airspeed->source = AIRSPEED_SOURCE_NONE;
        return;
    }

    gps_ground_speed_mps = AircraftMath_Clamp(gps_ground_speed_mps, 0.0f, 514.0f);
    target_mps = gps_ground_speed_mps;
    source = AIRSPEED_SOURCE_GPS_GROUND_SPEED;

    /*
     * Without wind data or a pitot/static system, this remains an estimate.
     * Prefer measured air density from BMP390 pressure and temperature, then
     * fall back to ISA density from pressure altitude, then raw GPS speed.
     */
    if (air_density_valid != 0u) {
        target_mps = gps_ground_speed_mps / sqrtf(density_ratio_from_pressure_temp(static_pressure_pa,
                                                                                   outside_air_temp_c));
        source = AIRSPEED_SOURCE_MEASURED_DENSITY_TRUE_AIRSPEED;
    } else if (pressure_altitude_valid != 0u) {
        target_mps = gps_ground_speed_mps / sqrtf(density_ratio_from_pressure_altitude(pressure_altitude_m));
        source = AIRSPEED_SOURCE_ESTIMATED_TRUE_AIRSPEED;
    }

    if (airspeed->valid == 0u) {
        airspeed->ground_speed_mps = gps_ground_speed_mps;
        airspeed->true_airspeed_mps = target_mps;
    } else {
        airspeed->ground_speed_mps = AircraftMath_LowPass(airspeed->ground_speed_mps,
                                                          gps_ground_speed_mps,
                                                          dt_s,
                                                          smoothing_tau_s);
        airspeed->true_airspeed_mps = AircraftMath_LowPass(airspeed->true_airspeed_mps,
                                                           target_mps,
                                                           dt_s,
                                                           smoothing_tau_s);
    }

    airspeed->true_airspeed_kt = airspeed->true_airspeed_mps * AIRSPEED_MPS_TO_KT;
    airspeed->display_true_airspeed_kt = airspeed_to_display_kt(airspeed->true_airspeed_kt);
    airspeed->source = source;
    airspeed->valid = 1u;
}
