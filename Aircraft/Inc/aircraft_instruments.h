#ifndef AIRCRAFT_INSTRUMENTS_H
#define AIRCRAFT_INSTRUMENTS_H

#include "altimeter.h"
#include "attitude_indicator.h"
#include "aircraft_math.h"
#include "heading_indicator.h"
#include "turn_slip.h"
#include "vertical_speed_indicator.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float gx_dps;
    float gy_dps;
    float gz_dps;
    float ax_g;
    float ay_g;
    float az_g;
    float altitude_m;
    float vertical_speed_mps;
    uint8_t mag_valid;
    uint8_t baro_valid;
} AircraftInstrumentsInput;

typedef struct
{
    AttitudeIndicatorData attitude;
    TurnSlipData turn_slip;
    HeadingData heading;
    AltimeterData altimeter;
    VerticalSpeedData vsi;
} AircraftInstrumentsOutput;

typedef struct
{
    float magnetic_declination_deg;
    float smoothing_tau_s;
    float altitude_smoothing_tau_s;
    float vsi_smoothing_tau_s;
    AircraftInstrumentsOutput output;
} AircraftInstruments;

void AircraftInstruments_Init(AircraftInstruments *inst,
                              float magnetic_declination_deg);

void AircraftInstruments_SetDeclination(AircraftInstruments *inst,
                                        float magnetic_declination_deg);

void AircraftInstruments_Update(AircraftInstruments *inst,
                                const AircraftInstrumentsInput *input,
                                float dt_s);

const AircraftInstrumentsOutput *AircraftInstruments_GetOutput(const AircraftInstruments *inst);

#ifdef __cplusplus
}
#endif

#endif
