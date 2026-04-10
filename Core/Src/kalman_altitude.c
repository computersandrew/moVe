#include "kalman_altitude.h"

#include <stddef.h>

#define KALMAN_ALTITUDE_DEFAULT_ACCEL_VAR       4.0f
#define KALMAN_ALTITUDE_DEFAULT_MEAS_VAR        1.5f
#define KALMAN_ALTITUDE_INITIAL_VARIANCE        25.0f
#define KALMAN_ALTITUDE_MIN_DT_S                0.001f
#define KALMAN_ALTITUDE_MAX_DT_S                1.0f

static float clamp_dt(float dt_s)
{
    if (dt_s < KALMAN_ALTITUDE_MIN_DT_S) {
        return KALMAN_ALTITUDE_MIN_DT_S;
    }
    if (dt_s > KALMAN_ALTITUDE_MAX_DT_S) {
        return KALMAN_ALTITUDE_MAX_DT_S;
    }

    return dt_s;
}

void KalmanAltitude_Init(KalmanAltitude *filter,
                         float initial_altitude_m)
{
    if (filter == NULL) {
        return;
    }

    filter->altitude_m = initial_altitude_m;
    filter->vertical_speed_mps = 0.0f;
    filter->p00 = KALMAN_ALTITUDE_INITIAL_VARIANCE;
    filter->p01 = 0.0f;
    filter->p10 = 0.0f;
    filter->p11 = KALMAN_ALTITUDE_INITIAL_VARIANCE;
    filter->process_accel_variance = KALMAN_ALTITUDE_DEFAULT_ACCEL_VAR;
    filter->measurement_variance = KALMAN_ALTITUDE_DEFAULT_MEAS_VAR;
    filter->initialized = 1u;
}

void KalmanAltitude_SetTuning(KalmanAltitude *filter,
                              float process_accel_variance,
                              float measurement_variance)
{
    if (filter == NULL) {
        return;
    }

    if (process_accel_variance > 0.0f) {
        filter->process_accel_variance = process_accel_variance;
    }

    if (measurement_variance > 0.0f) {
        filter->measurement_variance = measurement_variance;
    }
}

void KalmanAltitude_Reset(KalmanAltitude *filter,
                          float altitude_m)
{
    if (filter == NULL) {
        return;
    }

    KalmanAltitude_Init(filter, altitude_m);
}

void KalmanAltitude_Update(KalmanAltitude *filter,
                           float measured_altitude_m,
                           uint8_t measurement_valid,
                           float dt_s)
{
    float dt;
    float dt2;
    float dt3;
    float dt4;
    float q;
    float p00;
    float p01;
    float p10;
    float p11;

    if (filter == NULL) {
        return;
    }

    if (filter->initialized == 0u) {
        KalmanAltitude_Init(filter, measurement_valid ? measured_altitude_m : 0.0f);
    }

    dt = clamp_dt(dt_s);
    dt2 = dt * dt;
    dt3 = dt2 * dt;
    dt4 = dt2 * dt2;
    q = filter->process_accel_variance;

    filter->altitude_m += filter->vertical_speed_mps * dt;

    p00 = filter->p00 + (dt * (filter->p10 + filter->p01)) + (dt2 * filter->p11) + (0.25f * dt4 * q);
    p01 = filter->p01 + (dt * filter->p11) + (0.5f * dt3 * q);
    p10 = filter->p10 + (dt * filter->p11) + (0.5f * dt3 * q);
    p11 = filter->p11 + (dt2 * q);

    filter->p00 = p00;
    filter->p01 = p01;
    filter->p10 = p10;
    filter->p11 = p11;

    if (measurement_valid != 0u) {
        const float innovation = measured_altitude_m - filter->altitude_m;
        const float innovation_covariance = filter->p00 + filter->measurement_variance;
        const float k0 = filter->p00 / innovation_covariance;
        const float k1 = filter->p10 / innovation_covariance;

        p00 = filter->p00;
        p01 = filter->p01;
        p10 = filter->p10;
        p11 = filter->p11;

        filter->altitude_m += k0 * innovation;
        filter->vertical_speed_mps += k1 * innovation;
        filter->p00 = (1.0f - k0) * p00;
        filter->p01 = (1.0f - k0) * p01;
        filter->p10 = p10 - (k1 * p00);
        filter->p11 = p11 - (k1 * p01);
    }
}
