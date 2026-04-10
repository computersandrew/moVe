#ifndef BMP390_H
#define BMP390_H

#include "stm32_hal_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMP390_ADDR_I2C_PRIM              (0x76u << 1)
#define BMP390_ADDR_I2C_SEC               (0x77u << 1)
#define BMP390_SEA_LEVEL_PRESSURE_PA      101325.0f

typedef struct
{
    float pressure_pa;
    float temperature_c;
    float altitude_m;
    uint8_t data_ready;
} BMP390_Sample;

typedef struct
{
    double par_t1;
    double par_t2;
    double par_t3;
    double par_p1;
    double par_p2;
    double par_p3;
    double par_p4;
    double par_p5;
    double par_p6;
    double par_p7;
    double par_p8;
    double par_p9;
    double par_p10;
    double par_p11;
    double t_lin;
} BMP390_Calibration;

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t addr;
    float sea_level_pressure_pa;
    BMP390_Calibration calib;
} BMP390_HandleTypeDef;

HAL_StatusTypeDef BMP390_Init(BMP390_HandleTypeDef *dev,
                              I2C_HandleTypeDef *i2c,
                              uint8_t addr);

HAL_StatusTypeDef BMP390_Read(BMP390_HandleTypeDef *dev,
                              BMP390_Sample *sample);

HAL_StatusTypeDef BMP390_ReadForced(BMP390_HandleTypeDef *dev,
                                    BMP390_Sample *sample);

void BMP390_SetSeaLevelPressure(BMP390_HandleTypeDef *dev,
                                float sea_level_pressure_pa);

float BMP390_PressureToAltitudeM(float pressure_pa,
                                 float sea_level_pressure_pa);

#ifdef __cplusplus
}
#endif

#endif
