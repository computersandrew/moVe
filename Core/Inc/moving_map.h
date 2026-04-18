#ifndef MOVING_MAP_H
#define MOVING_MAP_H

#include "max_m10s.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOVING_MAP_DEFAULT_TILE_SIZE_PX    256u
#define MOVING_MAP_MAX_ZOOM                22u

typedef enum
{
    MOVING_MAP_PROVIDER_NONE = 0,
    MOVING_MAP_PROVIDER_FAA_XYZ = 1,
    MOVING_MAP_PROVIDER_GENERIC_XYZ = 2
} MovingMapProvider;

typedef struct
{
    MovingMapProvider provider;
    uint8_t zoom;
    uint16_t tile_size_px;
} MovingMapConfig;

typedef struct
{
    MovingMapProvider provider;
    uint8_t zoom;
    uint16_t tile_size_px;
    uint32_t tile_x;
    uint32_t tile_y;
    uint16_t pixel_x;
    uint16_t pixel_y;
    float latitude_deg;
    float longitude_deg;
    float course_deg;
    float ground_speed_mps;
    uint8_t fix_valid;
    uint8_t valid;
} MovingMapState;

typedef struct
{
    MovingMapConfig config;
    MovingMapState state;
} MovingMap;

void MovingMap_Init(MovingMap *map, const MovingMapConfig *config);
uint8_t MovingMap_UpdateFromGps(MovingMap *map, const MAXM10S_NavSample *gps);
const MovingMapState *MovingMap_GetState(const MovingMap *map);
uint8_t MovingMap_BuildTilePath(const MovingMapState *state,
                                const char *root,
                                const char *extension,
                                char *path,
                                uint16_t path_size);

#ifdef __cplusplus
}
#endif

#endif
