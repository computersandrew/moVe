#include "nora_b261.h"

#include <stddef.h>
#include <stdio.h>

static int32_t scaled_i32(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled >= 0.0f) {
        return (int32_t)(scaled + 0.5f);
    }

    return (int32_t)(scaled - 0.5f);
}

uint16_t NORA_B261_BuildMovingMapFrame(const MAXM10S_NavSample *gps,
                                       const MovingMapState *map_state,
                                       const char *tile_path,
                                       char *frame,
                                       uint16_t frame_size)
{
    int written;

    if ((gps == NULL) || (map_state == NULL) || (frame == NULL) ||
        (frame_size == 0u)) {
        return 0u;
    }

    if (tile_path == NULL) {
        tile_path = "";
    }

    written = snprintf(frame,
                       frame_size,
                       "$MOVE,MAP,%u,%ld,%ld,%ld,%ld,%ld,%u,%lu,%lu,%u,%u,%s\r\n",
                       (unsigned int)gps->fix_valid,
                       (long)scaled_i32(gps->latitude_deg, 10000000.0f),
                       (long)scaled_i32(gps->longitude_deg, 10000000.0f),
                       (long)scaled_i32(gps->altitude_m, 10.0f),
                       (long)scaled_i32(gps->ground_speed_mps, 100.0f),
                       (long)scaled_i32(gps->course_deg, 10.0f),
                       (unsigned int)map_state->zoom,
                       (unsigned long)map_state->tile_x,
                       (unsigned long)map_state->tile_y,
                       (unsigned int)map_state->pixel_x,
                       (unsigned int)map_state->pixel_y,
                       tile_path);

    if ((written <= 0) || ((uint16_t)written >= frame_size)) {
        return 0u;
    }

    return (uint16_t)written;
}
