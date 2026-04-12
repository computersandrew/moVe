#ifndef CREW_INSTRUMENTS_H
#define CREW_INSTRUMENTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CREW_ROLL_SET = 0,
    CREW_ROLL_PORT = 1,
    CREW_ROLL_STARBOARD = 2
} CrewRollState;

typedef enum
{
    CREW_STROKE_PHASE_UNKNOWN = 0,
    CREW_STROKE_PHASE_CATCH = 1,
    CREW_STROKE_PHASE_DRIVE = 2,
    CREW_STROKE_PHASE_RECOVERY = 3
} CrewStrokePhase;

typedef struct
{
    float roll_deg;
    float ax_g;
    float ay_g;
    float az_g;
    float gps_ground_speed_mps;
    float outside_air_temp_c;
    uint8_t gps_speed_valid;
    uint8_t temperature_valid;
} CrewInstrumentsInput;

typedef struct
{
    float roll_deg;
    CrewRollState roll_state;
    float speed_mps;
    float speed_kph;
    float speed_kt;
    float strokes_per_minute;
    CrewStrokePhase stroke_phase;
    float current_phase_time_s;
    float catch_time_s;
    float drive_time_s;
    float recovery_time_s;
    float drive_recovery_ratio;
    float recovery_drive_ratio;
    float stroke_cycle_time_s;
    float temperature_c;
    uint8_t speed_valid;
    uint8_t stroke_rate_valid;
    uint8_t stroke_timing_valid;
    uint8_t temperature_valid;
} CrewInstrumentsOutput;

typedef struct
{
    float roll_set_threshold_deg;
    float smoothing_tau_s;
    float speed_smoothing_tau_s;
    float stroke_smoothing_tau_s;
    float stroke_axis_baseline_g;
    float stroke_trigger_high_g;
    float stroke_trigger_low_g;
    float catch_hold_s;
    float min_stroke_interval_s;
    float max_stroke_interval_s;
    float seconds_since_stroke;
    float catch_time_s;
    float drive_time_s;
    float recovery_time_s;
    uint8_t stroke_armed;
    CrewStrokePhase stroke_phase;
    CrewInstrumentsOutput output;
} CrewInstruments;

void CrewInstruments_Init(CrewInstruments *crew);

void CrewInstruments_Update(CrewInstruments *crew,
                            const CrewInstrumentsInput *input,
                            float dt_s);

const CrewInstrumentsOutput *CrewInstruments_GetOutput(const CrewInstruments *crew);

const char *CrewInstruments_RollStateText(CrewRollState roll_state);
const char *CrewInstruments_StrokePhaseText(CrewStrokePhase stroke_phase);

#ifdef __cplusplus
}
#endif

#endif
