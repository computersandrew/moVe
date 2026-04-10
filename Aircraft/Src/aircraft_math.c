#include "aircraft_math.h"

float AircraftMath_Clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }

    return value;
}

float AircraftMath_Normalize360(float angle_deg)
{
    while (angle_deg >= 360.0f) {
        angle_deg -= 360.0f;
    }

    while (angle_deg < 0.0f) {
        angle_deg += 360.0f;
    }

    return angle_deg;
}

float AircraftMath_Wrap180(float angle_deg)
{
    angle_deg = AircraftMath_Normalize360(angle_deg);
    if (angle_deg > 180.0f) {
        angle_deg -= 360.0f;
    }

    return angle_deg;
}

float AircraftMath_LowPass(float previous, float target, float dt_s, float tau_s)
{
    float alpha;

    dt_s = AircraftMath_Clamp(dt_s, AIRCRAFT_MIN_DT_S, AIRCRAFT_MAX_DT_S);
    tau_s = (tau_s > 0.0f) ? tau_s : AIRCRAFT_DEFAULT_TAU_S;
    alpha = dt_s / (tau_s + dt_s);

    return previous + (alpha * (target - previous));
}

float AircraftMath_LowPassAngle360(float previous, float target, float dt_s, float tau_s)
{
    const float delta = AircraftMath_Wrap180(target - previous);

    return AircraftMath_Normalize360(AircraftMath_LowPass(previous, previous + delta, dt_s, tau_s));
}

uint16_t AircraftMath_HeadingToCard(float heading_deg)
{
    uint16_t heading = (uint16_t)(AircraftMath_Normalize360(heading_deg) + 0.5f);

    if (heading >= 360u) {
        heading = 0u;
    }

    return heading;
}

int32_t AircraftMath_AltitudeToDisplayFt(float altitude_ft)
{
    if (altitude_ft >= 0.0f) {
        return (int32_t)(altitude_ft + 0.5f);
    }

    return (int32_t)(altitude_ft - 0.5f);
}
