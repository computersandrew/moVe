#include "navigation_fusion.h"

#include <stddef.h>
#include <string.h>

#define NAV_FUSION_DEFAULT_SAMPLE_HZ      200.0f
#define NAV_FUSION_DEFAULT_BETA           0.08f

static float positive_or_default(float value, float default_value)
{
    return (value > 0.0f) ? value : default_value;
}

static void fill_aircraft_input(NavigationFusion *fusion,
                                const ICM20948_Sample *imu_sample,
                                uint8_t baro_display_valid,
                                AircraftInstrumentsInput *aircraft_input)
{
    aircraft_input->roll_deg = fusion->attitude_deg.roll_deg;
    aircraft_input->pitch_deg = fusion->attitude_deg.pitch_deg;
    aircraft_input->yaw_deg = fusion->attitude_deg.yaw_deg;
    aircraft_input->gx_dps = imu_sample->gx_dps;
    aircraft_input->gy_dps = imu_sample->gy_dps;
    aircraft_input->gz_dps = imu_sample->gz_dps;
    aircraft_input->ax_g = imu_sample->ax_g;
    aircraft_input->ay_g = imu_sample->ay_g;
    aircraft_input->az_g = imu_sample->az_g;
    aircraft_input->altitude_m = fusion->altitude_filter.altitude_m;
    aircraft_input->vertical_speed_mps = fusion->altitude_filter.vertical_speed_mps;
    aircraft_input->static_pressure_pa = fusion->baro_sample.pressure_pa;
    aircraft_input->outside_air_temp_c = fusion->baro_sample.temperature_c;
    aircraft_input->gps_ground_speed_mps = fusion->gps_sample.ground_speed_mps;
    aircraft_input->mag_valid = imu_sample->mag_data_valid;
    aircraft_input->baro_valid = baro_display_valid;
    aircraft_input->air_density_valid = baro_display_valid;
    aircraft_input->gps_speed_valid = fusion->gps_sample.speed_valid;
}

static void fill_crew_input(NavigationFusion *fusion,
                            const ICM20948_Sample *imu_sample,
                            uint8_t baro_display_valid,
                            CrewInstrumentsInput *crew_input)
{
    crew_input->roll_deg = fusion->attitude_deg.roll_deg;
    crew_input->ax_g = imu_sample->ax_g;
    crew_input->ay_g = imu_sample->ay_g;
    crew_input->az_g = imu_sample->az_g;
    crew_input->gps_ground_speed_mps = fusion->gps_sample.ground_speed_mps;
    crew_input->outside_air_temp_c = fusion->baro_sample.temperature_c;
    crew_input->gps_speed_valid = fusion->gps_sample.speed_valid;
    crew_input->temperature_valid = baro_display_valid;
}

HAL_StatusTypeDef NavigationFusion_Init(NavigationFusion *fusion,
                                        const NavigationFusion_Config *config)
{
    HAL_StatusTypeDef status;
    float sample_frequency_hz;
    float beta;

    if ((fusion == NULL) || (config == NULL) ||
        (config->imu_i2c == NULL) || (config->baro_i2c == NULL)) {
        return HAL_ERROR;
    }

    memset(fusion, 0, sizeof(*fusion));
    sample_frequency_hz = positive_or_default(config->sample_frequency_hz,
                                              NAV_FUSION_DEFAULT_SAMPLE_HZ);
    beta = positive_or_default(config->madgwick_beta, NAV_FUSION_DEFAULT_BETA);

    status = ICM20948_Init(&fusion->imu, config->imu_i2c, config->imu_addr);
    if (status != HAL_OK) {
        return status;
    }

    HAL_Delay(100u);
    status = ICM20948_CalibrateGyro(&fusion->imu, 300u, 2u);
    if (status != HAL_OK) {
        return status;
    }

    status = BMP390_Init(&fusion->baro, config->baro_i2c, config->baro_addr);
    if (status == HAL_OK) {
        fusion->baro_available = 1u;
        HAL_Delay(25u);
        status = BMP390_Read(&fusion->baro, &fusion->baro_sample);
        if ((status == HAL_OK) && (fusion->baro_sample.data_ready != 0u)) {
            KalmanAltitude_Init(&fusion->altitude_filter, fusion->baro_sample.altitude_m);
        } else {
            KalmanAltitude_Init(&fusion->altitude_filter, 0.0f);
        }
    } else {
        fusion->baro_available = 0u;
        KalmanAltitude_Init(&fusion->altitude_filter, 0.0f);
    }

    Madgwick_Init(&fusion->ahrs, sample_frequency_hz, beta);
    MAXM10S_Init(&fusion->gps_parser);
    AircraftInstruments_Init(&fusion->aircraft_instruments,
                             config->magnetic_declination_deg);
    CrewInstruments_Init(&fusion->crew_instruments);
    fusion->last_update_ms = HAL_GetTick();
    fusion->initialized = 1u;

    return HAL_OK;
}

HAL_StatusTypeDef NavigationFusion_Update(NavigationFusion *fusion)
{
    ICM20948_Sample imu_sample;
    AircraftInstrumentsInput aircraft_input;
    CrewInstrumentsInput crew_input;
    uint32_t now_ms;
    float dt_s;
    HAL_StatusTypeDef status;
    uint8_t baro_display_valid = 0u;
    uint8_t baro_measurement_valid = 0u;

    if ((fusion == NULL) || (fusion->initialized == 0u)) {
        return HAL_ERROR;
    }

    status = ICM20948_Read9Axis(&fusion->imu, &imu_sample);
    if (status != HAL_OK) {
        return status;
    }

    now_ms = HAL_GetTick();
    dt_s = (float)(now_ms - fusion->last_update_ms) * 0.001f;
    fusion->last_update_ms = now_ms;

    if (dt_s > 0.0f) {
        Madgwick_SetSamplePeriod(&fusion->ahrs, dt_s);
    }

    if (imu_sample.mag_data_valid != 0u) {
        Madgwick_Update(&fusion->ahrs,
                        imu_sample.gx_dps * ICM20948_DEG_TO_RAD,
                        imu_sample.gy_dps * ICM20948_DEG_TO_RAD,
                        imu_sample.gz_dps * ICM20948_DEG_TO_RAD,
                        imu_sample.ax_g,
                        imu_sample.ay_g,
                        imu_sample.az_g,
                        imu_sample.mx_uT,
                        imu_sample.my_uT,
                        imu_sample.mz_uT);
    } else {
        Madgwick_UpdateIMU(&fusion->ahrs,
                           imu_sample.gx_dps * ICM20948_DEG_TO_RAD,
                           imu_sample.gy_dps * ICM20948_DEG_TO_RAD,
                           imu_sample.gz_dps * ICM20948_DEG_TO_RAD,
                           imu_sample.ax_g,
                           imu_sample.ay_g,
                           imu_sample.az_g);
    }

    Madgwick_GetEulerDeg(&fusion->ahrs,
                         &fusion->attitude_deg.roll_deg,
                         &fusion->attitude_deg.pitch_deg,
                         &fusion->attitude_deg.yaw_deg);

    if (fusion->baro_available != 0u) {
        status = BMP390_Read(&fusion->baro, &fusion->baro_sample);
        if (status == HAL_OK) {
            baro_display_valid = 1u;
            if (fusion->baro_sample.data_ready != 0u) {
                baro_measurement_valid = 1u;
            }
        } else {
            fusion->baro_available = 0u;
        }
    }

    KalmanAltitude_Update(&fusion->altitude_filter,
                          fusion->baro_sample.altitude_m,
                          baro_measurement_valid,
                          dt_s);

    fill_aircraft_input(fusion, &imu_sample, baro_display_valid, &aircraft_input);
    AircraftInstruments_Update(&fusion->aircraft_instruments, &aircraft_input, dt_s);

    fill_crew_input(fusion, &imu_sample, baro_display_valid, &crew_input);
    CrewInstruments_Update(&fusion->crew_instruments, &crew_input, dt_s);

    return HAL_OK;
}

uint8_t NavigationFusion_ProcessGpsByte(NavigationFusion *fusion,
                                        uint8_t byte)
{
    if (fusion == NULL) {
        return MAXM10S_UPDATE_NONE;
    }

    return MAXM10S_ProcessByte(&fusion->gps_parser, byte, &fusion->gps_sample);
}

uint8_t NavigationFusion_ProcessGpsBuffer(NavigationFusion *fusion,
                                          const uint8_t *data,
                                          uint16_t length)
{
    if (fusion == NULL) {
        return MAXM10S_UPDATE_NONE;
    }

    return MAXM10S_ProcessBuffer(&fusion->gps_parser, data, length, &fusion->gps_sample);
}

void NavigationFusion_SetSeaLevelPressure(NavigationFusion *fusion,
                                          float sea_level_pressure_pa)
{
    if (fusion == NULL) {
        return;
    }

    BMP390_SetSeaLevelPressure(&fusion->baro, sea_level_pressure_pa);
}

void NavigationFusion_SetMagneticDeclination(NavigationFusion *fusion,
                                             float magnetic_declination_deg)
{
    if (fusion == NULL) {
        return;
    }

    AircraftInstruments_SetDeclination(&fusion->aircraft_instruments,
                                       magnetic_declination_deg);
}

const AircraftInstrumentsOutput *NavigationFusion_GetAircraftDisplay(const NavigationFusion *fusion)
{
    if (fusion == NULL) {
        return NULL;
    }

    return AircraftInstruments_GetOutput(&fusion->aircraft_instruments);
}

const CrewInstrumentsOutput *NavigationFusion_GetCrewDisplay(const NavigationFusion *fusion)
{
    if (fusion == NULL) {
        return NULL;
    }

    return CrewInstruments_GetOutput(&fusion->crew_instruments);
}

const AttitudeDeg *NavigationFusion_GetAttitude(const NavigationFusion *fusion)
{
    if (fusion == NULL) {
        return NULL;
    }

    return &fusion->attitude_deg;
}

const BMP390_Sample *NavigationFusion_GetBaroSample(const NavigationFusion *fusion)
{
    if (fusion == NULL) {
        return NULL;
    }

    return &fusion->baro_sample;
}

const MAXM10S_NavSample *NavigationFusion_GetGpsSample(const NavigationFusion *fusion)
{
    if (fusion == NULL) {
        return NULL;
    }

    return &fusion->gps_sample;
}
