#include "mq7_co.h"

#include "aircraft_math.h"

#include <stddef.h>

#define MQ7_CO_MAX_DISPLAY_PPM      1000.0f

static MQ7COAlertLevel alert_for_ppm(float co_ppm)
{
    if (co_ppm >= MQ7_CO_DANGER_PPM) {
        return MQ7_CO_ALERT_100_PPM;
    }
    if (co_ppm >= MQ7_CO_WARNING_PPM) {
        return MQ7_CO_ALERT_50_PPM;
    }
    if (co_ppm >= MQ7_CO_CAUTION_PPM) {
        return MQ7_CO_ALERT_25_PPM;
    }

    return MQ7_CO_ALERT_CLEAR;
}

void MQ7CO_Init(MQ7COData *data)
{
    if (data == NULL) {
        return;
    }

    data->co_ppm = 0.0f;
    data->sensor_voltage_v = 0.0f;
    data->alert_level = MQ7_CO_ALERT_CLEAR;
    data->trigger_25ppm = 0u;
    data->trigger_50ppm = 0u;
    data->trigger_100ppm = 0u;
    data->valid = 0u;
}

void MQ7CO_UpdateFromPpm(MQ7COData *data, float co_ppm)
{
    MQ7COAlertLevel alert_level;

    if (data == NULL) {
        return;
    }

    co_ppm = AircraftMath_Clamp(co_ppm, 0.0f, MQ7_CO_MAX_DISPLAY_PPM);
    alert_level = alert_for_ppm(co_ppm);

    data->co_ppm = co_ppm;
    data->alert_level = alert_level;
    data->trigger_25ppm = (co_ppm >= MQ7_CO_CAUTION_PPM) ? 1u : 0u;
    data->trigger_50ppm = (co_ppm >= MQ7_CO_WARNING_PPM) ? 1u : 0u;
    data->trigger_100ppm = (co_ppm >= MQ7_CO_DANGER_PPM) ? 1u : 0u;
    data->valid = 1u;
}

uint8_t MQ7CO_UpdateFromVoltage(MQ7COData *data,
                                float sensor_voltage_v,
                                const MQ7COCalibration *calibration)
{
    float co_ppm;

    if ((data == NULL) || (calibration == NULL) ||
        (calibration->ppm_per_volt <= 0.0f)) {
        return 0u;
    }

    data->sensor_voltage_v = sensor_voltage_v;
    co_ppm = (sensor_voltage_v - calibration->zero_voltage_v) *
             calibration->ppm_per_volt;
    MQ7CO_UpdateFromPpm(data, co_ppm);

    return 1u;
}

MQ7COHeaterPhase MQ7CO_GetHeaterPhase(uint32_t cycle_seconds)
{
    const uint32_t cycle_len_s = MQ7_HEATER_CLEAN_S + MQ7_HEATER_MEASURE_S;
    const uint32_t phase_s = (cycle_len_s > 0u) ? (cycle_seconds % cycle_len_s) : 0u;

    if (phase_s < MQ7_HEATER_CLEAN_S) {
        return MQ7_HEATER_PHASE_CLEAN;
    }

    return MQ7_HEATER_PHASE_MEASURE;
}

float MQ7CO_GetHeaterVoltage(MQ7COHeaterPhase phase)
{
    if (phase == MQ7_HEATER_PHASE_MEASURE) {
        return 1.5f;
    }

    return 5.0f;
}

const char *MQ7CO_AlertText(MQ7COAlertLevel alert_level)
{
    switch (alert_level) {
    case MQ7_CO_ALERT_25_PPM:
        return "CO 25 ppm";
    case MQ7_CO_ALERT_50_PPM:
        return "CO 50 ppm";
    case MQ7_CO_ALERT_100_PPM:
        return "CO 100 ppm";
    case MQ7_CO_ALERT_CLEAR:
    default:
        return "CO clear";
    }
}
