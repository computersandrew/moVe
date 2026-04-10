#ifndef ICM20948_MADGWICK_EXAMPLE_H
#define ICM20948_MADGWICK_EXAMPLE_H

#include "icm20948.h"
#include "aircraft_instruments.h"
#include "bmp390.h"
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

extern AttitudeDeg attitude_deg;
extern AircraftInstruments aircraft_instruments;
extern const AircraftInstrumentsOutput *aircraft_display;
extern BMP390_Sample baro_sample;
extern MAXM10S_NavSample gps_sample;

HAL_StatusTypeDef Attitude_Init(void);
HAL_StatusTypeDef Attitude_Update(void);
void Navigation_GpsProcessByte(uint8_t byte);
void Navigation_GpsProcessBuffer(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
