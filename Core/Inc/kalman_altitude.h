#ifndef KALMAN_ALTITUDE_H
#define KALMAN_ALTITUDE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float altitude_m;
    float vertical_speed_mps;
    float p00;
    float p01;
    float p10;
    float p11;
    float process_accel_variance;
    float measurement_variance;
    uint8_t initialized;
} KalmanAltitude;

void KalmanAltitude_Init(KalmanAltitude *filter,
                         float initial_altitude_m);

void KalmanAltitude_SetTuning(KalmanAltitude *filter,
                              float process_accel_variance,
                              float measurement_variance);

void KalmanAltitude_Reset(KalmanAltitude *filter,
                          float altitude_m);

void KalmanAltitude_Update(KalmanAltitude *filter,
                           float measured_altitude_m,
                           uint8_t measurement_valid,
                           float dt_s);

#ifdef __cplusplus
}
#endif

#endif
