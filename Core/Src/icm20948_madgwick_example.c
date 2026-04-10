#include "icm20948_madgwick_example.h"

#include "icm20948.h"
#include "madgwick.h"

extern I2C_HandleTypeDef hi2c1;

static ICM20948_HandleTypeDef imu;
static MadgwickAHRS ahrs;
static uint32_t last_update_ms;

AttitudeDeg attitude_deg;
AircraftInstruments aircraft_instruments;
const AircraftInstrumentsOutput *aircraft_display;

HAL_StatusTypeDef Attitude_Init(void)
{
    HAL_StatusTypeDef status;

    status = ICM20948_Init(&imu, &hi2c1, ICM20948_ADDR_AD0_LOW);
    if (status != HAL_OK) {
        return status;
    }

    HAL_Delay(100u);
    status = ICM20948_CalibrateGyro(&imu, 300u, 2u);
    if (status != HAL_OK) {
        return status;
    }

    Madgwick_Init(&ahrs, 200.0f, 0.08f);
    AircraftInstruments_Init(&aircraft_instruments, 0.0f);
    aircraft_display = AircraftInstruments_GetOutput(&aircraft_instruments);
    last_update_ms = HAL_GetTick();

    return HAL_OK;
}

HAL_StatusTypeDef Attitude_Update(void)
{
    ICM20948_Sample sample;
    uint32_t now_ms;
    float dt_s;
    HAL_StatusTypeDef status;

    status = ICM20948_Read9Axis(&imu, &sample);
    if (status != HAL_OK) {
        return status;
    }

    now_ms = HAL_GetTick();
    dt_s = (float)(now_ms - last_update_ms) * 0.001f;
    last_update_ms = now_ms;

    if (dt_s > 0.0f) {
        Madgwick_SetSamplePeriod(&ahrs, dt_s);
    }

    if (sample.mag_data_valid != 0u) {
        Madgwick_Update(&ahrs,
                        sample.gx_dps * ICM20948_DEG_TO_RAD,
                        sample.gy_dps * ICM20948_DEG_TO_RAD,
                        sample.gz_dps * ICM20948_DEG_TO_RAD,
                        sample.ax_g,
                        sample.ay_g,
                        sample.az_g,
                        sample.mx_uT,
                        sample.my_uT,
                        sample.mz_uT);
    } else {
        Madgwick_UpdateIMU(&ahrs,
                           sample.gx_dps * ICM20948_DEG_TO_RAD,
                           sample.gy_dps * ICM20948_DEG_TO_RAD,
                           sample.gz_dps * ICM20948_DEG_TO_RAD,
                           sample.ax_g,
                           sample.ay_g,
                           sample.az_g);
    }

    Madgwick_GetEulerDeg(&ahrs,
                         &attitude_deg.roll_deg,
                         &attitude_deg.pitch_deg,
                         &attitude_deg.yaw_deg);

    {
        AircraftInstrumentsInput aircraft_input;

        aircraft_input.roll_deg = attitude_deg.roll_deg;
        aircraft_input.pitch_deg = attitude_deg.pitch_deg;
        aircraft_input.yaw_deg = attitude_deg.yaw_deg;
        aircraft_input.gx_dps = sample.gx_dps;
        aircraft_input.gy_dps = sample.gy_dps;
        aircraft_input.gz_dps = sample.gz_dps;
        aircraft_input.ax_g = sample.ax_g;
        aircraft_input.ay_g = sample.ay_g;
        aircraft_input.az_g = sample.az_g;
        aircraft_input.mag_valid = sample.mag_data_valid;

        AircraftInstruments_Update(&aircraft_instruments, &aircraft_input, dt_s);
        aircraft_display = AircraftInstruments_GetOutput(&aircraft_instruments);
    }

    return HAL_OK;
}
