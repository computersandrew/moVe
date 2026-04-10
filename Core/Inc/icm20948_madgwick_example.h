#ifndef ICM20948_MADGWICK_EXAMPLE_H
#define ICM20948_MADGWICK_EXAMPLE_H

#include "icm20948.h"
#include "aircraft_instruments.h"

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

HAL_StatusTypeDef Attitude_Init(void);
HAL_StatusTypeDef Attitude_Update(void);

#ifdef __cplusplus
}
#endif

#endif
