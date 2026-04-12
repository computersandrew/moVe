#include "icm20948_madgwick_example.h"

extern I2C_HandleTypeDef hi2c1;

NavigationFusion navigation_fusion;
AttitudeDeg attitude_deg;
AircraftInstruments aircraft_instruments;
const AircraftInstrumentsOutput *aircraft_display;
CrewInstruments crew_instruments;
const CrewInstrumentsOutput *crew_display;
BMP390_Sample baro_sample;
MAXM10S_NavSample gps_sample;

static void sync_legacy_outputs(void)
{
    const AttitudeDeg *attitude = NavigationFusion_GetAttitude(&navigation_fusion);
    const BMP390_Sample *baro = NavigationFusion_GetBaroSample(&navigation_fusion);
    const MAXM10S_NavSample *gps = NavigationFusion_GetGpsSample(&navigation_fusion);

    if (attitude != 0) {
        attitude_deg = *attitude;
    }

    aircraft_instruments = navigation_fusion.aircraft_instruments;
    aircraft_display = AircraftInstruments_GetOutput(&aircraft_instruments);
    crew_instruments = navigation_fusion.crew_instruments;
    crew_display = CrewInstruments_GetOutput(&crew_instruments);

    if (baro != 0) {
        baro_sample = *baro;
    }

    if (gps != 0) {
        gps_sample = *gps;
    }
}

HAL_StatusTypeDef Attitude_Init(void)
{
    NavigationFusion_Config config;
    HAL_StatusTypeDef status;

    config.imu_i2c = &hi2c1;
    config.imu_addr = ICM20948_ADDR_AD0_LOW;
    config.baro_i2c = &hi2c1;
    config.baro_addr = BMP390_ADDR_I2C_PRIM;
    config.sample_frequency_hz = 200.0f;
    config.madgwick_beta = 0.08f;
    config.magnetic_declination_deg = 0.0f;

    status = NavigationFusion_Init(&navigation_fusion, &config);
    sync_legacy_outputs();

    return status;
}

HAL_StatusTypeDef Attitude_Update(void)
{
    HAL_StatusTypeDef status = NavigationFusion_Update(&navigation_fusion);

    sync_legacy_outputs();

    return status;
}

void Navigation_GpsProcessByte(uint8_t byte)
{
    (void)NavigationFusion_ProcessGpsByte(&navigation_fusion, byte);
    sync_legacy_outputs();
}

void Navigation_GpsProcessBuffer(const uint8_t *data, uint16_t length)
{
    (void)NavigationFusion_ProcessGpsBuffer(&navigation_fusion, data, length);
    sync_legacy_outputs();
}
