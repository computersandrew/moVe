#ifndef ALTIMETER_H
#define ALTIMETER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float altitude_m;
    float altitude_ft;
    int32_t display_altitude_ft;
    uint8_t valid;
} AltimeterData;

void Altimeter_Init(AltimeterData *altimeter);
void Altimeter_Update(AltimeterData *altimeter,
                      float altitude_m,
                      uint8_t baro_valid,
                      float dt_s,
                      float smoothing_tau_s);

#ifdef __cplusplus
}
#endif

#endif
