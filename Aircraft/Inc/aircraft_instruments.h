#ifndef AIRCRAFT_INSTRUMENTS_H
#define AIRCRAFT_INSTRUMENTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AIRCRAFT_STANDARD_RATE_TURN_DPS    3.0f

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
    uint8_t mag_valid;
} AircraftInstrumentsInput;

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float bank_pointer_deg;
    uint8_t valid;
} AttitudeIndicatorData;

typedef struct
{
    float turn_rate_deg_s;
    float standard_rate_fraction;
    float slip_ball;
    uint8_t valid;
} TurnSlipData;

typedef struct
{
    float magnetic_heading_deg;
    float true_heading_deg;
    uint16_t display_heading_deg;
    uint8_t magnetic_valid;
} HeadingData;

typedef struct
{
    AttitudeIndicatorData attitude;
    TurnSlipData turn_slip;
    HeadingData heading;
} AircraftInstrumentsOutput;

typedef struct
{
    float magnetic_declination_deg;
    float smoothing_tau_s;
    AircraftInstrumentsOutput output;
    uint8_t initialized;
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
