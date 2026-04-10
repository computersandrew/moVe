#ifndef MADGWICK_H
#define MADGWICK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float q0;
    float q1;
    float q2;
    float q3;
    float beta;
    float sample_period_s;
} MadgwickAHRS;

void Madgwick_Init(MadgwickAHRS *filter, float sample_frequency_hz, float beta);
void Madgwick_SetSampleFrequency(MadgwickAHRS *filter, float sample_frequency_hz);
void Madgwick_SetSamplePeriod(MadgwickAHRS *filter, float sample_period_s);

void Madgwick_Update(MadgwickAHRS *filter,
                     float gx_rad_s, float gy_rad_s, float gz_rad_s,
                     float ax_g, float ay_g, float az_g,
                     float mx_uT, float my_uT, float mz_uT);

void Madgwick_UpdateIMU(MadgwickAHRS *filter,
                        float gx_rad_s, float gy_rad_s, float gz_rad_s,
                        float ax_g, float ay_g, float az_g);

void Madgwick_GetQuaternion(const MadgwickAHRS *filter,
                            float *q0, float *q1, float *q2, float *q3);

void Madgwick_GetEulerDeg(const MadgwickAHRS *filter,
                          float *roll_deg, float *pitch_deg, float *yaw_deg);

#ifdef __cplusplus
}
#endif

#endif
