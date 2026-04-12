#include "crew_instruments.h"

#include <stddef.h>

#define CREW_DEFAULT_TAU_S                 0.25f
#define CREW_MIN_DT_S                      0.001f
#define CREW_MAX_DT_S                      0.250f
#define CREW_MPS_TO_KPH                    3.6f
#define CREW_MPS_TO_KT                     1.94384449f
#define CREW_NO_STROKE_TIMEOUT_S           6.0f

static float clampf(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }

    return value;
}

static float low_pass(float previous, float target, float dt_s, float tau_s)
{
    float alpha;

    dt_s = clampf(dt_s, CREW_MIN_DT_S, CREW_MAX_DT_S);
    tau_s = (tau_s > 0.0f) ? tau_s : CREW_DEFAULT_TAU_S;
    alpha = dt_s / (tau_s + dt_s);

    return previous + (alpha * (target - previous));
}

static CrewRollState roll_state_from_deg(float roll_deg, float set_threshold_deg)
{
    const float threshold = (set_threshold_deg > 0.0f) ? set_threshold_deg : 2.0f;

    if (roll_deg > threshold) {
        return CREW_ROLL_STARBOARD;
    }
    if (roll_deg < -threshold) {
        return CREW_ROLL_PORT;
    }

    return CREW_ROLL_SET;
}

static void update_speed(CrewInstruments *crew,
                         const CrewInstrumentsInput *input,
                         float dt_s)
{
    if (input->gps_speed_valid != 0u) {
        const float speed_mps = clampf(input->gps_ground_speed_mps, 0.0f, 30.0f);

        if (crew->output.speed_valid == 0u) {
            crew->output.speed_mps = speed_mps;
        } else {
            crew->output.speed_mps = low_pass(crew->output.speed_mps,
                                              speed_mps,
                                              dt_s,
                                              crew->speed_smoothing_tau_s);
        }
        crew->output.speed_valid = 1u;
    } else {
        crew->output.speed_valid = 0u;
    }

    crew->output.speed_kph = crew->output.speed_mps * CREW_MPS_TO_KPH;
    crew->output.speed_kt = crew->output.speed_mps * CREW_MPS_TO_KT;
}

static void update_stroke_rate(CrewInstruments *crew,
                               const CrewInstrumentsInput *input,
                               float dt_s)
{
    const float dynamic_accel_g = input->ax_g - crew->stroke_axis_baseline_g;

    crew->seconds_since_stroke += clampf(dt_s, CREW_MIN_DT_S, CREW_MAX_DT_S);
    crew->stroke_axis_baseline_g = low_pass(crew->stroke_axis_baseline_g,
                                            input->ax_g,
                                            dt_s,
                                            1.25f);

    if (dynamic_accel_g < crew->stroke_trigger_low_g) {
        crew->stroke_armed = 1u;
    }

    if ((crew->stroke_armed != 0u) &&
        (dynamic_accel_g > crew->stroke_trigger_high_g)) {
        const float interval_s = crew->seconds_since_stroke;

        crew->stroke_armed = 0u;
        crew->seconds_since_stroke = 0.0f;

        if ((interval_s >= crew->min_stroke_interval_s) &&
            (interval_s <= crew->max_stroke_interval_s)) {
            const float spm = 60.0f / interval_s;

            if (crew->output.stroke_rate_valid == 0u) {
                crew->output.strokes_per_minute = spm;
            } else {
                crew->output.strokes_per_minute =
                    low_pass(crew->output.strokes_per_minute,
                             spm,
                             interval_s,
                             crew->stroke_smoothing_tau_s);
            }
            crew->output.stroke_rate_valid = 1u;
        }
    }

    if (crew->seconds_since_stroke > CREW_NO_STROKE_TIMEOUT_S) {
        crew->output.strokes_per_minute =
            low_pass(crew->output.strokes_per_minute,
                     0.0f,
                     dt_s,
                     crew->stroke_smoothing_tau_s);
        crew->output.stroke_rate_valid = 0u;
    }
}

void CrewInstruments_Init(CrewInstruments *crew)
{
    if (crew == NULL) {
        return;
    }

    crew->roll_set_threshold_deg = 2.0f;
    crew->smoothing_tau_s = CREW_DEFAULT_TAU_S;
    crew->speed_smoothing_tau_s = 0.60f;
    crew->stroke_smoothing_tau_s = 1.50f;
    crew->stroke_axis_baseline_g = 0.0f;
    crew->stroke_trigger_high_g = 0.16f;
    crew->stroke_trigger_low_g = 0.04f;
    crew->min_stroke_interval_s = 0.75f;
    crew->max_stroke_interval_s = 4.00f;
    crew->seconds_since_stroke = CREW_NO_STROKE_TIMEOUT_S;
    crew->stroke_armed = 1u;

    crew->output.roll_deg = 0.0f;
    crew->output.roll_state = CREW_ROLL_SET;
    crew->output.speed_mps = 0.0f;
    crew->output.speed_kph = 0.0f;
    crew->output.speed_kt = 0.0f;
    crew->output.strokes_per_minute = 0.0f;
    crew->output.temperature_c = 0.0f;
    crew->output.speed_valid = 0u;
    crew->output.stroke_rate_valid = 0u;
    crew->output.temperature_valid = 0u;
}

void CrewInstruments_Update(CrewInstruments *crew,
                            const CrewInstrumentsInput *input,
                            float dt_s)
{
    if ((crew == NULL) || (input == NULL)) {
        return;
    }

    crew->output.roll_deg = low_pass(crew->output.roll_deg,
                                     input->roll_deg,
                                     dt_s,
                                     crew->smoothing_tau_s);
    crew->output.roll_state =
        roll_state_from_deg(crew->output.roll_deg, crew->roll_set_threshold_deg);

    update_speed(crew, input, dt_s);
    update_stroke_rate(crew, input, dt_s);

    if (input->temperature_valid != 0u) {
        if (crew->output.temperature_valid == 0u) {
            crew->output.temperature_c = input->outside_air_temp_c;
        } else {
            crew->output.temperature_c = low_pass(crew->output.temperature_c,
                                                 input->outside_air_temp_c,
                                                 dt_s,
                                                 crew->smoothing_tau_s);
        }
        crew->output.temperature_valid = 1u;
    } else {
        crew->output.temperature_valid = 0u;
    }
}

const CrewInstrumentsOutput *CrewInstruments_GetOutput(const CrewInstruments *crew)
{
    if (crew == NULL) {
        return NULL;
    }

    return &crew->output;
}

const char *CrewInstruments_RollStateText(CrewRollState roll_state)
{
    switch (roll_state) {
    case CREW_ROLL_PORT:
        return "Port";
    case CREW_ROLL_STARBOARD:
        return "Starboard";
    case CREW_ROLL_SET:
    default:
        return "Set";
    }
}
