#ifndef NAVIGATION_FUSION_H
#define NAVIGATION_FUSION_H

#include "aircraft_instruments.h"
#include "bmp390.h"
#include "crew_instruments.h"
#include "icm20948.h"
#include "kalman_altitude.h"
#include "madgwick.h"
#include "max_m10s.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
} AttitudeDeg;

typedef struct
{
    I2C_HandleTypeDef *imu_i2c;
    uint8_t imu_addr;
    I2C_HandleTypeDef *baro_i2c;
    uint8_t baro_addr;
    float sample_frequency_hz;
    float madgwick_beta;
    float magnetic_declination_deg;
} NavigationFusion_Config;

typedef struct
{
    ICM20948_HandleTypeDef imu;
    BMP390_HandleTypeDef baro;
    MAXM10S_Parser gps_parser;
    MadgwickAHRS ahrs;
    KalmanAltitude altitude_filter;
    AircraftInstruments aircraft_instruments;
    CrewInstruments crew_instruments;
    AttitudeDeg attitude_deg;
    BMP390_Sample baro_sample;
    MAXM10S_NavSample gps_sample;
    uint32_t last_update_ms;
    uint8_t baro_available;
    uint8_t initialized;
} NavigationFusion;

HAL_StatusTypeDef NavigationFusion_Init(NavigationFusion *fusion,
                                        const NavigationFusion_Config *config);

HAL_StatusTypeDef NavigationFusion_Update(NavigationFusion *fusion);

uint8_t NavigationFusion_ProcessGpsByte(NavigationFusion *fusion,
                                        uint8_t byte);

uint8_t NavigationFusion_ProcessGpsBuffer(NavigationFusion *fusion,
                                          const uint8_t *data,
                                          uint16_t length);

void NavigationFusion_SetSeaLevelPressure(NavigationFusion *fusion,
                                          float sea_level_pressure_pa);

void NavigationFusion_SetMagneticDeclination(NavigationFusion *fusion,
                                             float magnetic_declination_deg);

const AircraftInstrumentsOutput *NavigationFusion_GetAircraftDisplay(const NavigationFusion *fusion);
const CrewInstrumentsOutput *NavigationFusion_GetCrewDisplay(const NavigationFusion *fusion);
const AttitudeDeg *NavigationFusion_GetAttitude(const NavigationFusion *fusion);
const BMP390_Sample *NavigationFusion_GetBaroSample(const NavigationFusion *fusion);
const MAXM10S_NavSample *NavigationFusion_GetGpsSample(const NavigationFusion *fusion);

#ifdef __cplusplus
}
#endif

#endif
