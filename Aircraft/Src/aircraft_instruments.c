#include "aircraft_instruments.h"

#include <math.h>
#include <stddef.h>

#define AIRCRAFT_DEFAULT_TAU_S       0.18f
#define AIRCRAFT_MAX_BANK_DEG        180.0f
#define AIRCRAFT_MAX_PITCH_DEG       90.0f
#define AIRCRAFT_MAX_TURN_DPS        12.0f
#define AIRCRAFT_MAX_SLIP_BALL       1.0f
#define AIRCRAFT_MIN_DT_S            0.001f
#define AIRCRAFT_MAX_DT_S            0.200f

static float clampf_local(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }

    return value;
}

static float normalize_360(float angle_deg)
{
    while (angle_deg >= 360.0f) {
        angle_deg -= 360.0f;
    }

    while (angle_deg < 0.0f) {
        angle_deg += 360.0f;
    }

    return angle_deg;
}

static float wrap_180(float angle_deg)
{
    angle_deg = normalize_360(angle_deg);
    if (angle_deg > 180.0f) {
        angle_deg -= 360.0f;
    }

    return angle_deg;
}

static float low_pass(float previous, float target, float dt_s, float tau_s)
{
    float alpha;

    dt_s = clampf_local(dt_s, AIRCRAFT_MIN_DT_S, AIRCRAFT_MAX_DT_S);
    tau_s = (tau_s > 0.0f) ? tau_s : AIRCRAFT_DEFAULT_TAU_S;
    alpha = dt_s / (tau_s + dt_s);

    return previous + (alpha * (target - previous));
}

static float low_pass_angle_360(float previous, float target, float dt_s, float tau_s)
{
    const float delta = wrap_180(target - previous);

    return normalize_360(low_pass(previous, previous + delta, dt_s, tau_s));
}

static uint16_t heading_to_card(float heading_deg)
{
    uint16_t heading = (uint16_t)(normalize_360(heading_deg) + 0.5f);

    if (heading >= 360u) {
        heading = 0u;
    }

    return heading;
}

void AircraftInstruments_Init(AircraftInstruments *inst,
                              float magnetic_declination_deg)
{
    if (inst == NULL) {
        return;
    }

    inst->magnetic_declination_deg = magnetic_declination_deg;
    inst->smoothing_tau_s = AIRCRAFT_DEFAULT_TAU_S;
    inst->output.attitude.roll_deg = 0.0f;
    inst->output.attitude.pitch_deg = 0.0f;
    inst->output.attitude.bank_pointer_deg = 0.0f;
    inst->output.attitude.valid = 0u;
    inst->output.turn_slip.turn_rate_deg_s = 0.0f;
    inst->output.turn_slip.standard_rate_fraction = 0.0f;
    inst->output.turn_slip.slip_ball = 0.0f;
    inst->output.turn_slip.valid = 0u;
    inst->output.heading.magnetic_heading_deg = 0.0f;
    inst->output.heading.true_heading_deg = normalize_360(magnetic_declination_deg);
    inst->output.heading.display_heading_deg = heading_to_card(inst->output.heading.true_heading_deg);
    inst->output.heading.magnetic_valid = 0u;
    inst->initialized = 0u;
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
    float roll;
    float pitch;
    float turn_rate;
    float slip_ball;
    float magnetic_heading;
    float true_heading;

    if ((inst == NULL) || (input == NULL)) {
        return;
    }

    roll = clampf_local(wrap_180(input->roll_deg), -AIRCRAFT_MAX_BANK_DEG, AIRCRAFT_MAX_BANK_DEG);
    pitch = clampf_local(input->pitch_deg, -AIRCRAFT_MAX_PITCH_DEG, AIRCRAFT_MAX_PITCH_DEG);

    turn_rate = clampf_local(input->gz_dps, -AIRCRAFT_MAX_TURN_DPS, AIRCRAFT_MAX_TURN_DPS);

    /*
     * Slip ball convention: positive moves the ball right. Most aircraft panels
     * should invert this at the drawing layer if their screen coordinate grows
     * to the right but the ball graphic is modeled as "step on the ball".
     */
    slip_ball = clampf_local(input->ay_g, -AIRCRAFT_MAX_SLIP_BALL, AIRCRAFT_MAX_SLIP_BALL);

    magnetic_heading = normalize_360(input->yaw_deg);
    true_heading = normalize_360(magnetic_heading + inst->magnetic_declination_deg);

    if (inst->initialized == 0u) {
        inst->output.attitude.roll_deg = roll;
        inst->output.attitude.pitch_deg = pitch;
        inst->output.attitude.bank_pointer_deg = roll;
        inst->output.turn_slip.turn_rate_deg_s = turn_rate;
        inst->output.turn_slip.slip_ball = slip_ball;
        inst->output.heading.magnetic_heading_deg = magnetic_heading;
        inst->output.heading.true_heading_deg = true_heading;
        inst->initialized = 1u;
    } else {
        inst->output.attitude.roll_deg = low_pass(inst->output.attitude.roll_deg,
                                                  roll,
                                                  dt_s,
                                                  inst->smoothing_tau_s);
        inst->output.attitude.pitch_deg = low_pass(inst->output.attitude.pitch_deg,
                                                   pitch,
                                                   dt_s,
                                                   inst->smoothing_tau_s);
        inst->output.attitude.bank_pointer_deg = inst->output.attitude.roll_deg;
        inst->output.turn_slip.turn_rate_deg_s = low_pass(inst->output.turn_slip.turn_rate_deg_s,
                                                          turn_rate,
                                                          dt_s,
                                                          inst->smoothing_tau_s);
        inst->output.turn_slip.slip_ball = low_pass(inst->output.turn_slip.slip_ball,
                                                    slip_ball,
                                                    dt_s,
                                                    inst->smoothing_tau_s);

        if (input->mag_valid != 0u) {
            inst->output.heading.magnetic_heading_deg =
                low_pass_angle_360(inst->output.heading.magnetic_heading_deg,
                                   magnetic_heading,
                                   dt_s,
                                   inst->smoothing_tau_s);
            inst->output.heading.true_heading_deg =
                low_pass_angle_360(inst->output.heading.true_heading_deg,
                                   true_heading,
                                   dt_s,
                                   inst->smoothing_tau_s);
        }
    }

    inst->output.attitude.valid = 1u;
    inst->output.turn_slip.standard_rate_fraction =
        clampf_local(inst->output.turn_slip.turn_rate_deg_s / AIRCRAFT_STANDARD_RATE_TURN_DPS,
                     -1.0f,
                     1.0f);
    inst->output.turn_slip.valid = 1u;
    inst->output.heading.magnetic_valid = input->mag_valid;
    inst->output.heading.display_heading_deg = heading_to_card(inst->output.heading.true_heading_deg);
}

const AircraftInstrumentsOutput *AircraftInstruments_GetOutput(const AircraftInstruments *inst)
{
    if (inst == NULL) {
        return NULL;
    }

    return &inst->output;
}
