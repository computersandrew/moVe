#ifndef MQ7_CO_H
#define MQ7_CO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQ7_CO_CAUTION_PPM       25.0f
#define MQ7_CO_WARNING_PPM       50.0f
#define MQ7_CO_DANGER_PPM        100.0f

#define MQ7_HEATER_CLEAN_S       60u
#define MQ7_HEATER_MEASURE_S     90u

typedef enum
{
    MQ7_CO_ALERT_CLEAR = 0,
    MQ7_CO_ALERT_25_PPM = 1,
    MQ7_CO_ALERT_50_PPM = 2,
    MQ7_CO_ALERT_100_PPM = 3
} MQ7COAlertLevel;

typedef enum
{
    MQ7_HEATER_PHASE_CLEAN = 0,
    MQ7_HEATER_PHASE_MEASURE = 1
} MQ7COHeaterPhase;

typedef struct
{
    float zero_voltage_v;
    float ppm_per_volt;
} MQ7COCalibration;

typedef struct
{
    float co_ppm;
    float sensor_voltage_v;
    MQ7COAlertLevel alert_level;
    uint8_t trigger_25ppm;
    uint8_t trigger_50ppm;
    uint8_t trigger_100ppm;
    uint8_t valid;
} MQ7COData;

void MQ7CO_Init(MQ7COData *data);
void MQ7CO_UpdateFromPpm(MQ7COData *data, float co_ppm);
uint8_t MQ7CO_UpdateFromVoltage(MQ7COData *data,
                                float sensor_voltage_v,
                                const MQ7COCalibration *calibration);
MQ7COHeaterPhase MQ7CO_GetHeaterPhase(uint32_t cycle_seconds);
float MQ7CO_GetHeaterVoltage(MQ7COHeaterPhase phase);
const char *MQ7CO_AlertText(MQ7COAlertLevel alert_level);

#ifdef __cplusplus
}
#endif

#endif
