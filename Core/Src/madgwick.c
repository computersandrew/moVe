#include "madgwick.h"

#include <math.h>

#define MADGWICK_DEFAULT_BETA        0.08f
#define MADGWICK_DEFAULT_PERIOD_S    0.005f
#define MADGWICK_RAD_TO_DEG          57.29577951308232f

static float inv_sqrt(float x)
{
    if (x <= 0.0f) {
        return 0.0f;
    }

    return 1.0f / sqrtf(x);
}

static void normalize_quaternion(MadgwickAHRS *filter)
{
    const float recip_norm = inv_sqrt((filter->q0 * filter->q0) +
                                      (filter->q1 * filter->q1) +
                                      (filter->q2 * filter->q2) +
                                      (filter->q3 * filter->q3));

    if (recip_norm == 0.0f) {
        filter->q0 = 1.0f;
        filter->q1 = 0.0f;
        filter->q2 = 0.0f;
        filter->q3 = 0.0f;
        return;
    }

    filter->q0 *= recip_norm;
    filter->q1 *= recip_norm;
    filter->q2 *= recip_norm;
    filter->q3 *= recip_norm;
}

void Madgwick_Init(MadgwickAHRS *filter, float sample_frequency_hz, float beta)
{
    if (filter == 0) {
        return;
    }

    filter->q0 = 1.0f;
    filter->q1 = 0.0f;
    filter->q2 = 0.0f;
    filter->q3 = 0.0f;
    filter->beta = (beta > 0.0f) ? beta : MADGWICK_DEFAULT_BETA;
    Madgwick_SetSampleFrequency(filter, sample_frequency_hz);
}

void Madgwick_SetSampleFrequency(MadgwickAHRS *filter, float sample_frequency_hz)
{
    if (filter == 0) {
        return;
    }

    filter->sample_period_s = (sample_frequency_hz > 0.0f)
                                  ? (1.0f / sample_frequency_hz)
                                  : MADGWICK_DEFAULT_PERIOD_S;
}

void Madgwick_SetSamplePeriod(MadgwickAHRS *filter, float sample_period_s)
{
    if (filter == 0) {
        return;
    }

    filter->sample_period_s = (sample_period_s > 0.0f) ? sample_period_s : MADGWICK_DEFAULT_PERIOD_S;
}

void Madgwick_UpdateIMU(MadgwickAHRS *filter,
                        float gx_rad_s, float gy_rad_s, float gz_rad_s,
                        float ax_g, float ay_g, float az_g)
{
    float q0;
    float q1;
    float q2;
    float q3;
    float q_dot0;
    float q_dot1;
    float q_dot2;
    float q_dot3;
    float recip_norm;
    float s0;
    float s1;
    float s2;
    float s3;
    float q0q0;
    float q1q1;
    float q2q2;
    float q3q3;
    float two_q0;
    float two_q1;
    float two_q2;
    float two_q3;
    float four_q0;
    float four_q1;
    float four_q2;
    float eight_q1;
    float eight_q2;

    if (filter == 0) {
        return;
    }

    q0 = filter->q0;
    q1 = filter->q1;
    q2 = filter->q2;
    q3 = filter->q3;

    q_dot0 = 0.5f * ((-q1 * gx_rad_s) - (q2 * gy_rad_s) - (q3 * gz_rad_s));
    q_dot1 = 0.5f * ((q0 * gx_rad_s) + (q2 * gz_rad_s) - (q3 * gy_rad_s));
    q_dot2 = 0.5f * ((q0 * gy_rad_s) - (q1 * gz_rad_s) + (q3 * gx_rad_s));
    q_dot3 = 0.5f * ((q0 * gz_rad_s) + (q1 * gy_rad_s) - (q2 * gx_rad_s));

    if (!((ax_g == 0.0f) && (ay_g == 0.0f) && (az_g == 0.0f))) {
        recip_norm = inv_sqrt((ax_g * ax_g) + (ay_g * ay_g) + (az_g * az_g));
        ax_g *= recip_norm;
        ay_g *= recip_norm;
        az_g *= recip_norm;

        two_q0 = 2.0f * q0;
        two_q1 = 2.0f * q1;
        two_q2 = 2.0f * q2;
        two_q3 = 2.0f * q3;
        four_q0 = 4.0f * q0;
        four_q1 = 4.0f * q1;
        four_q2 = 4.0f * q2;
        eight_q1 = 8.0f * q1;
        eight_q2 = 8.0f * q2;
        q0q0 = q0 * q0;
        q1q1 = q1 * q1;
        q2q2 = q2 * q2;
        q3q3 = q3 * q3;

        s0 = (four_q0 * q2q2) + (two_q2 * ax_g) + (four_q0 * q1q1) - (two_q1 * ay_g);
        s1 = (four_q1 * q3q3) - (two_q3 * ax_g) + (4.0f * q0q0 * q1) -
             (two_q0 * ay_g) - four_q1 + (eight_q1 * q1q1) + (eight_q1 * q2q2) +
             (four_q1 * az_g);
        s2 = (4.0f * q0q0 * q2) + (two_q0 * ax_g) + (four_q2 * q3q3) -
             (two_q3 * ay_g) - four_q2 + (eight_q2 * q1q1) + (eight_q2 * q2q2) +
             (four_q2 * az_g);
        s3 = (4.0f * q1q1 * q3) - (two_q1 * ax_g) + (4.0f * q2q2 * q3) -
             (two_q2 * ay_g);

        recip_norm = inv_sqrt((s0 * s0) + (s1 * s1) + (s2 * s2) + (s3 * s3));
        if (recip_norm > 0.0f) {
            s0 *= recip_norm;
            s1 *= recip_norm;
            s2 *= recip_norm;
            s3 *= recip_norm;

            q_dot0 -= filter->beta * s0;
            q_dot1 -= filter->beta * s1;
            q_dot2 -= filter->beta * s2;
            q_dot3 -= filter->beta * s3;
        }
    }

    filter->q0 += q_dot0 * filter->sample_period_s;
    filter->q1 += q_dot1 * filter->sample_period_s;
    filter->q2 += q_dot2 * filter->sample_period_s;
    filter->q3 += q_dot3 * filter->sample_period_s;
    normalize_quaternion(filter);
}

void Madgwick_Update(MadgwickAHRS *filter,
                     float gx_rad_s, float gy_rad_s, float gz_rad_s,
                     float ax_g, float ay_g, float az_g,
                     float mx_uT, float my_uT, float mz_uT)
{
    float q0;
    float q1;
    float q2;
    float q3;
    float q_dot0;
    float q_dot1;
    float q_dot2;
    float q_dot3;
    float recip_norm;
    float s0;
    float s1;
    float s2;
    float s3;
    float hx;
    float hy;
    float two_bx;
    float two_bz;
    float four_bx;
    float four_bz;
    float two_q0mx;
    float two_q0my;
    float two_q0mz;
    float two_q1mx;
    float two_q0;
    float two_q1;
    float two_q2;
    float two_q3;
    float two_q0q2;
    float two_q2q3;
    float q0q0;
    float q0q1;
    float q0q2;
    float q0q3;
    float q1q1;
    float q1q2;
    float q1q3;
    float q2q2;
    float q2q3;
    float q3q3;

    if (filter == 0) {
        return;
    }

    if ((mx_uT == 0.0f) && (my_uT == 0.0f) && (mz_uT == 0.0f)) {
        Madgwick_UpdateIMU(filter, gx_rad_s, gy_rad_s, gz_rad_s, ax_g, ay_g, az_g);
        return;
    }

    q0 = filter->q0;
    q1 = filter->q1;
    q2 = filter->q2;
    q3 = filter->q3;

    q_dot0 = 0.5f * ((-q1 * gx_rad_s) - (q2 * gy_rad_s) - (q3 * gz_rad_s));
    q_dot1 = 0.5f * ((q0 * gx_rad_s) + (q2 * gz_rad_s) - (q3 * gy_rad_s));
    q_dot2 = 0.5f * ((q0 * gy_rad_s) - (q1 * gz_rad_s) + (q3 * gx_rad_s));
    q_dot3 = 0.5f * ((q0 * gz_rad_s) + (q1 * gy_rad_s) - (q2 * gx_rad_s));

    if (!((ax_g == 0.0f) && (ay_g == 0.0f) && (az_g == 0.0f))) {
        recip_norm = inv_sqrt((ax_g * ax_g) + (ay_g * ay_g) + (az_g * az_g));
        ax_g *= recip_norm;
        ay_g *= recip_norm;
        az_g *= recip_norm;

        recip_norm = inv_sqrt((mx_uT * mx_uT) + (my_uT * my_uT) + (mz_uT * mz_uT));
        mx_uT *= recip_norm;
        my_uT *= recip_norm;
        mz_uT *= recip_norm;

        two_q0mx = 2.0f * q0 * mx_uT;
        two_q0my = 2.0f * q0 * my_uT;
        two_q0mz = 2.0f * q0 * mz_uT;
        two_q1mx = 2.0f * q1 * mx_uT;
        two_q0 = 2.0f * q0;
        two_q1 = 2.0f * q1;
        two_q2 = 2.0f * q2;
        two_q3 = 2.0f * q3;
        two_q0q2 = 2.0f * q0 * q2;
        two_q2q3 = 2.0f * q2 * q3;
        q0q0 = q0 * q0;
        q0q1 = q0 * q1;
        q0q2 = q0 * q2;
        q0q3 = q0 * q3;
        q1q1 = q1 * q1;
        q1q2 = q1 * q2;
        q1q3 = q1 * q3;
        q2q2 = q2 * q2;
        q2q3 = q2 * q3;
        q3q3 = q3 * q3;

        hx = (mx_uT * q0q0) - (two_q0my * q3) + (two_q0mz * q2) +
             (mx_uT * q1q1) + (two_q1 * my_uT * q2) + (two_q1 * mz_uT * q3) -
             (mx_uT * q2q2) - (mx_uT * q3q3);
        hy = (two_q0mx * q3) + (my_uT * q0q0) - (two_q0mz * q1) +
             (two_q1mx * q2) - (my_uT * q1q1) + (my_uT * q2q2) +
             (two_q2 * mz_uT * q3) - (my_uT * q3q3);
        two_bx = sqrtf((hx * hx) + (hy * hy));
        two_bz = (-two_q0mx * q2) + (two_q0my * q1) + (mz_uT * q0q0) +
                 (two_q1mx * q3) - (mz_uT * q1q1) + (two_q2 * my_uT * q3) -
                 (mz_uT * q2q2) + (mz_uT * q3q3);
        four_bx = 2.0f * two_bx;
        four_bz = 2.0f * two_bz;

        s0 = (-two_q2 * ((2.0f * q1q3) - two_q0q2 - ax_g)) +
             (two_q1 * ((2.0f * q0q1) + two_q2q3 - ay_g)) -
             (two_bz * q2 * ((two_bx * ((0.5f - q2q2) - q3q3)) +
                              (two_bz * (q1q3 - q0q2)) - mx_uT)) +
             (((-two_bx * q3) + (two_bz * q1)) *
              ((two_bx * (q1q2 - q0q3)) + (two_bz * (q0q1 + q2q3)) - my_uT)) +
             (two_bx * q2 *
              ((two_bx * (q0q2 + q1q3)) + (two_bz * ((0.5f - q1q1) - q2q2)) - mz_uT));

        s1 = (two_q3 * ((2.0f * q1q3) - two_q0q2 - ax_g)) +
             (two_q0 * ((2.0f * q0q1) + two_q2q3 - ay_g)) -
             (4.0f * q1 * ((1.0f - (2.0f * q1q1) - (2.0f * q2q2)) - az_g)) +
             (two_bz * q3 *
              ((two_bx * ((0.5f - q2q2) - q3q3)) + (two_bz * (q1q3 - q0q2)) - mx_uT)) +
             (((two_bx * q2) + (two_bz * q0)) *
              ((two_bx * (q1q2 - q0q3)) + (two_bz * (q0q1 + q2q3)) - my_uT)) +
             (((two_bx * q3) - (four_bz * q1)) *
              ((two_bx * (q0q2 + q1q3)) + (two_bz * ((0.5f - q1q1) - q2q2)) - mz_uT));

        s2 = (-two_q0 * ((2.0f * q1q3) - two_q0q2 - ax_g)) +
             (two_q3 * ((2.0f * q0q1) + two_q2q3 - ay_g)) -
             (4.0f * q2 * ((1.0f - (2.0f * q1q1) - (2.0f * q2q2)) - az_g)) +
             (((-four_bx * q2) - (two_bz * q0)) *
              ((two_bx * ((0.5f - q2q2) - q3q3)) + (two_bz * (q1q3 - q0q2)) - mx_uT)) +
             (((two_bx * q1) + (two_bz * q3)) *
              ((two_bx * (q1q2 - q0q3)) + (two_bz * (q0q1 + q2q3)) - my_uT)) +
             (((two_bx * q0) - (four_bz * q2)) *
              ((two_bx * (q0q2 + q1q3)) + (two_bz * ((0.5f - q1q1) - q2q2)) - mz_uT));

        s3 = (two_q1 * ((2.0f * q1q3) - two_q0q2 - ax_g)) +
             (two_q2 * ((2.0f * q0q1) + two_q2q3 - ay_g)) +
             (((-four_bx * q3) + (two_bz * q1)) *
              ((two_bx * ((0.5f - q2q2) - q3q3)) + (two_bz * (q1q3 - q0q2)) - mx_uT)) +
             (((-two_bx * q0) + (two_bz * q2)) *
              ((two_bx * (q1q2 - q0q3)) + (two_bz * (q0q1 + q2q3)) - my_uT)) +
             (two_bx * q1 *
              ((two_bx * (q0q2 + q1q3)) + (two_bz * ((0.5f - q1q1) - q2q2)) - mz_uT));

        recip_norm = inv_sqrt((s0 * s0) + (s1 * s1) + (s2 * s2) + (s3 * s3));
        if (recip_norm > 0.0f) {
            s0 *= recip_norm;
            s1 *= recip_norm;
            s2 *= recip_norm;
            s3 *= recip_norm;

            q_dot0 -= filter->beta * s0;
            q_dot1 -= filter->beta * s1;
            q_dot2 -= filter->beta * s2;
            q_dot3 -= filter->beta * s3;
        }
    }

    filter->q0 += q_dot0 * filter->sample_period_s;
    filter->q1 += q_dot1 * filter->sample_period_s;
    filter->q2 += q_dot2 * filter->sample_period_s;
    filter->q3 += q_dot3 * filter->sample_period_s;
    normalize_quaternion(filter);
}

void Madgwick_GetQuaternion(const MadgwickAHRS *filter,
                            float *q0, float *q1, float *q2, float *q3)
{
    if (filter == 0) {
        return;
    }

    if (q0 != 0) {
        *q0 = filter->q0;
    }
    if (q1 != 0) {
        *q1 = filter->q1;
    }
    if (q2 != 0) {
        *q2 = filter->q2;
    }
    if (q3 != 0) {
        *q3 = filter->q3;
    }
}

void Madgwick_GetEulerDeg(const MadgwickAHRS *filter,
                          float *roll_deg, float *pitch_deg, float *yaw_deg)
{
    float q0;
    float q1;
    float q2;
    float q3;
    float sin_pitch;

    if (filter == 0) {
        return;
    }

    q0 = filter->q0;
    q1 = filter->q1;
    q2 = filter->q2;
    q3 = filter->q3;

    if (roll_deg != 0) {
        *roll_deg = atan2f(2.0f * ((q0 * q1) + (q2 * q3)),
                           1.0f - (2.0f * ((q1 * q1) + (q2 * q2)))) *
                    MADGWICK_RAD_TO_DEG;
    }

    if (pitch_deg != 0) {
        sin_pitch = 2.0f * ((q0 * q2) - (q3 * q1));
        if (sin_pitch > 1.0f) {
            sin_pitch = 1.0f;
        } else if (sin_pitch < -1.0f) {
            sin_pitch = -1.0f;
        }
        *pitch_deg = asinf(sin_pitch) * MADGWICK_RAD_TO_DEG;
    }

    if (yaw_deg != 0) {
        *yaw_deg = atan2f(2.0f * ((q0 * q3) + (q1 * q2)),
                          1.0f - (2.0f * ((q2 * q2) + (q3 * q3)))) *
                   MADGWICK_RAD_TO_DEG;
    }
}
