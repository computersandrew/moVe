#include "aircraft_instruments.h"

#include "aircraft_math.h"

#include <stddef.h>

void AircraftInstruments_Init(AircraftInstruments *inst,
                              float magnetic_declination_deg)
{
    if (inst == NULL) {
        return;
    }

    inst->magnetic_declination_deg = magnetic_declination_deg;
    inst->smoothing_tau_s = AIRCRAFT_DEFAULT_TAU_S;
    inst->altitude_smoothing_tau_s = 0.35f;
    inst->vsi_smoothing_tau_s = 0.75f;

    AttitudeIndicator_Init(&inst->output.attitude);
    TurnSlip_Init(&inst->output.turn_slip);
    HeadingIndicator_Init(&inst->output.heading, magnetic_declination_deg);
    Altimeter_Init(&inst->output.altimeter);
    VerticalSpeedIndicator_Init(&inst->output.vsi);
}

void AircraftInstruments_SetDeclination(AircraftInstruments *inst,
                                        float magnetic_declination_deg)
{
    if (inst == NULL) {
        return;
    }

    inst->magnetic_declination_deg = magnetic_declination_deg;
}

void AircraftInstruments_Update(AircraftInstruments *inst,
                                const AircraftInstrumentsInput *input,
                                float dt_s)
{
    if ((inst == NULL) || (input == NULL)) {
        return;
    }

    AttitudeIndicator_Update(&inst->output.attitude,
                             input->roll_deg,
                             input->pitch_deg,
                             dt_s,
                             inst->smoothing_tau_s);

    TurnSlip_Update(&inst->output.turn_slip,
                    input->gz_dps,
                    input->ay_g,
                    dt_s,
                    inst->smoothing_tau_s);

    HeadingIndicator_Update(&inst->output.heading,
                            input->yaw_deg,
                            input->mag_valid,
                            inst->magnetic_declination_deg,
                            dt_s,
                            inst->smoothing_tau_s);

    Altimeter_Update(&inst->output.altimeter,
                     input->altitude_m,
                     input->baro_valid,
                     dt_s,
                     inst->altitude_smoothing_tau_s);

    VerticalSpeedIndicator_Update(&inst->output.vsi,
                                  input->vertical_speed_mps,
                                  input->baro_valid,
                                  dt_s,
                                  inst->vsi_smoothing_tau_s);
}

const AircraftInstrumentsOutput *AircraftInstruments_GetOutput(const AircraftInstruments *inst)
{
    if (inst == NULL) {
        return NULL;
    }

    return &inst->output;
}
