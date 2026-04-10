#ifndef AIRSPEED_INDICATOR_H
#define AIRSPEED_INDICATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    AIRSPEED_SOURCE_NONE = 0,
    AIRSPEED_SOURCE_GPS_GROUND_SPEED = 1,
    AIRSPEED_SOURCE_ESTIMATED_TRUE_AIRSPEED = 2
} AirspeedSource;

typedef struct
{
    float ground_speed_mps;
    float true_airspeed_mps;
    float true_airspeed_kt;
    uint16_t display_true_airspeed_kt;
    AirspeedSource source;
    uint8_t valid;
} AirspeedData;

#define indicated_airspeed_mps       true_airspeed_mps
#define indicated_airspeed_kt        true_airspeed_kt
#define display_airspeed_kt          display_true_airspeed_kt
#define AIRSPEED_SOURCE_GPS_PRESSURE_ALTITUDE_ESTIMATE    AIRSPEED_SOURCE_ESTIMATED_TRUE_AIRSPEED

void AirspeedIndicator_Init(AirspeedData *airspeed);
void AirspeedIndicator_Update(AirspeedData *airspeed,
                              float gps_ground_speed_mps,
                              uint8_t gps_speed_valid,
                              float pressure_altitude_m,
                              uint8_t pressure_altitude_valid,
                              float dt_s,
                              float smoothing_tau_s);

#ifdef __cplusplus
}
#endif

#endif
