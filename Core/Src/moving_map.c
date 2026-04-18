#include "moving_map.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#define MOVING_MAP_PI                 3.14159265358979323846f
#define MOVING_MAP_MIN_LAT_DEG        -85.05112878f
#define MOVING_MAP_MAX_LAT_DEG        85.05112878f

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

static uint8_t normalized_zoom(uint8_t zoom)
{
    return (zoom > MOVING_MAP_MAX_ZOOM) ? MOVING_MAP_MAX_ZOOM : zoom;
}

static uint32_t tile_count_for_zoom(uint8_t zoom)
{
    return 1UL << normalized_zoom(zoom);
}

void MovingMap_Init(MovingMap *map, const MovingMapConfig *config)
{
    MovingMapConfig default_config;

    if (map == NULL) {
        return;
    }

    default_config.provider = MOVING_MAP_PROVIDER_FAA_XYZ;
    default_config.zoom = 10u;
    default_config.tile_size_px = MOVING_MAP_DEFAULT_TILE_SIZE_PX;

    if (config == NULL) {
        map->config = default_config;
    } else {
        map->config = *config;
    }

    map->config.zoom = normalized_zoom(map->config.zoom);
    if (map->config.tile_size_px == 0u) {
        map->config.tile_size_px = MOVING_MAP_DEFAULT_TILE_SIZE_PX;
    }

    map->state.provider = map->config.provider;
    map->state.zoom = map->config.zoom;
    map->state.tile_size_px = map->config.tile_size_px;
    map->state.tile_x = 0u;
    map->state.tile_y = 0u;
    map->state.pixel_x = 0u;
    map->state.pixel_y = 0u;
    map->state.latitude_deg = 0.0f;
    map->state.longitude_deg = 0.0f;
    map->state.course_deg = 0.0f;
    map->state.ground_speed_mps = 0.0f;
    map->state.fix_valid = 0u;
    map->state.valid = 0u;
}

uint8_t MovingMap_UpdateFromGps(MovingMap *map, const MAXM10S_NavSample *gps)
{
    const uint32_t tile_count = (map != NULL) ? tile_count_for_zoom(map->config.zoom) : 0u;
    const float tile_count_f = (float)tile_count;
    float latitude_deg;
    float longitude_deg;
    float latitude_rad;
    float tile_x_f;
    float tile_y_f;
    uint32_t tile_x;
    uint32_t tile_y;

    if ((map == NULL) || (gps == NULL) || (gps->fix_valid == 0u) ||
        (tile_count == 0u)) {
        if (map != NULL) {
            map->state.fix_valid = 0u;
            map->state.valid = 0u;
        }
        return 0u;
    }

    latitude_deg = clampf(gps->latitude_deg,
                          MOVING_MAP_MIN_LAT_DEG,
                          MOVING_MAP_MAX_LAT_DEG);
    longitude_deg = clampf(gps->longitude_deg, -180.0f, 180.0f);
    latitude_rad = latitude_deg * (MOVING_MAP_PI / 180.0f);

    tile_x_f = ((longitude_deg + 180.0f) / 360.0f) * tile_count_f;
    tile_y_f = (1.0f - (logf(tanf(latitude_rad) +
                             (1.0f / cosf(latitude_rad))) / MOVING_MAP_PI)) *
               0.5f * tile_count_f;

    tile_x_f = clampf(tile_x_f, 0.0f, tile_count_f - 0.0001f);
    tile_y_f = clampf(tile_y_f, 0.0f, tile_count_f - 0.0001f);
    tile_x = (uint32_t)floorf(tile_x_f);
    tile_y = (uint32_t)floorf(tile_y_f);

    map->state.provider = map->config.provider;
    map->state.zoom = map->config.zoom;
    map->state.tile_size_px = map->config.tile_size_px;
    map->state.tile_x = tile_x;
    map->state.tile_y = tile_y;
    map->state.pixel_x =
        (uint16_t)((tile_x_f - (float)tile_x) * (float)map->config.tile_size_px);
    map->state.pixel_y =
        (uint16_t)((tile_y_f - (float)tile_y) * (float)map->config.tile_size_px);
    map->state.latitude_deg = latitude_deg;
    map->state.longitude_deg = longitude_deg;
    map->state.course_deg = gps->course_deg;
    map->state.ground_speed_mps = gps->ground_speed_mps;
    map->state.fix_valid = gps->fix_valid;
    map->state.valid = 1u;

    return 1u;
}

const MovingMapState *MovingMap_GetState(const MovingMap *map)
{
    if (map == NULL) {
        return NULL;
    }

    return &map->state;
}

uint8_t MovingMap_BuildTilePath(const MovingMapState *state,
                                const char *root,
                                const char *extension,
                                char *path,
                                uint16_t path_size)
{
    int written;

    if ((state == NULL) || (path == NULL) || (path_size == 0u) ||
        (state->valid == 0u)) {
        return 0u;
    }

    if (root == NULL) {
        root = "maps";
    }
    if (extension == NULL) {
        extension = "tile";
    }

    written = snprintf(path,
                       path_size,
                       "%s/%u/%lu/%lu.%s",
                       root,
                       (unsigned int)state->zoom,
                       (unsigned long)state->tile_x,
                       (unsigned long)state->tile_y,
                       extension);

    return ((written > 0) && ((uint16_t)written < path_size)) ? 1u : 0u;
}
