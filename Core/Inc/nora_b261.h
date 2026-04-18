#ifndef NORA_B261_H
#define NORA_B261_H

#include "max_m10s.h"
#include "moving_map.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NORA_B261_DEFAULT_BAUD       115200u

/*
 * Builds a line-oriented payload for the NORA-B261 UART/u-connectXpress path:
 * $MOVE,MAP,fix,lat_e7,lon_e7,alt_dm,speed_cmps,course_deci,zoom,tile_x,tile_y,pixel_x,pixel_y,path
 */
uint16_t NORA_B261_BuildMovingMapFrame(const MAXM10S_NavSample *gps,
                                       const MovingMapState *map_state,
                                       const char *tile_path,
                                       char *frame,
                                       uint16_t frame_size);

#ifdef __cplusplus
}
#endif

#endif
