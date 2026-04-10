#include "icm20948_madgwick_example.h"

#include "bmp390.h"
#include "icm20948.h"
#include "kalman_altitude.h"
#include "max_m10s.h"
#include "madgwick.h"

extern I2C_HandleTypeDef hi2c1;

static ICM20948_HandleTypeDef imu;
static BMP390_HandleTypeDef baro;
static MAXM10S_Parser gps_parser;
static MadgwickAHRS ahrs;
static KalmanAltitude altitude_filter;
static uint32_t last_update_ms;
static uint8_t baro_available;

AttitudeDeg attitude_deg;
AircraftInstruments aircraft_instruments;
const AircraftInstrumentsOutput *aircraft_display;
BMP390_Sample baro_sample;
MAXM10S_NavSample gps_sample;

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

    status = BMP390_Init(&baro, &hi2c1, BMP390_ADDR_I2C_PRIM);
    if (status == HAL_OK) {
        baro_available = 1u;
        HAL_Delay(25u);
        status = BMP390_Read(&baro, &baro_sample);
        if ((status == HAL_OK) && (baro_sample.data_ready != 0u)) {
            KalmanAltitude_Init(&altitude_filter, baro_sample.altitude_m);
        } else {
            KalmanAltitude_Init(&altitude_filter, 0.0f);
        }
    } else {
        baro_available = 0u;
        KalmanAltitude_Init(&altitude_filter, 0.0f);
    }

    Madgwick_Init(&ahrs, 200.0f, 0.08f);
    MAXM10S_Init(&gps_parser);
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
        uint8_t baro_display_valid = 0u;
        uint8_t baro_valid = 0u;

        if (baro_available != 0u) {
            status = BMP390_Read(&baro, &baro_sample);
            if (status == HAL_OK) {
                baro_display_valid = 1u;
                if (baro_sample.data_ready != 0u) {
                    baro_valid = 1u;
                }
            } else {
                baro_available = 0u;
            }
        }

        KalmanAltitude_Update(&altitude_filter,
                              baro_sample.altitude_m,
                              baro_valid,
                              dt_s);

        aircraft_input.roll_deg = attitude_deg.roll_deg;
        aircraft_input.pitch_deg = attitude_deg.pitch_deg;
        aircraft_input.yaw_deg = attitude_deg.yaw_deg;
        aircraft_input.gx_dps = sample.gx_dps;
        aircraft_input.gy_dps = sample.gy_dps;
        aircraft_input.gz_dps = sample.gz_dps;
        aircraft_input.ax_g = sample.ax_g;
        aircraft_input.ay_g = sample.ay_g;
        aircraft_input.az_g = sample.az_g;
        aircraft_input.altitude_m = altitude_filter.altitude_m;
        aircraft_input.vertical_speed_mps = altitude_filter.vertical_speed_mps;
        aircraft_input.gps_ground_speed_mps = gps_sample.ground_speed_mps;
        aircraft_input.mag_valid = sample.mag_data_valid;
        aircraft_input.baro_valid = baro_display_valid;
        aircraft_input.gps_speed_valid = gps_sample.speed_valid;

        AircraftInstruments_Update(&aircraft_instruments, &aircraft_input, dt_s);
        aircraft_display = AircraftInstruments_GetOutput(&aircraft_instruments);
    }

    return HAL_OK;
}

void Navigation_GpsProcessByte(uint8_t byte)
{
    (void)MAXM10S_ProcessByte(&gps_parser, byte, &gps_sample);
}

void Navigation_GpsProcessBuffer(const uint8_t *data, uint16_t length)
{
    (void)MAXM10S_ProcessBuffer(&gps_parser, data, length, &gps_sample);
}
