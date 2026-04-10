#ifndef AIRCRAFT_MATH_H
#define AIRCRAFT_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AIRCRAFT_DEFAULT_TAU_S       0.18f
#define AIRCRAFT_MIN_DT_S            0.001f
#define AIRCRAFT_MAX_DT_S            0.200f
#define AIRCRAFT_M_TO_FT             3.280839895f
#define AIRCRAFT_MPS_TO_FPM          196.8503937f

float AircraftMath_Clamp(float value, float min_value, float max_value);
float AircraftMath_Normalize360(float angle_deg);
float AircraftMath_Wrap180(float angle_deg);
float AircraftMath_LowPass(float previous, float target, float dt_s, float tau_s);
float AircraftMath_LowPassAngle360(float previous, float target, float dt_s, float tau_s);
uint16_t AircraftMath_HeadingToCard(float heading_deg);
int32_t AircraftMath_AltitudeToDisplayFt(float altitude_ft);

#ifdef __cplusplus
}
#endif

#endif
