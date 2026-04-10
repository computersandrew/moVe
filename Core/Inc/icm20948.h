#ifndef ICM20948_H
#define ICM20948_H

#include <stdint.h>

#if defined(ICM20948_HOST_BUILD)
typedef struct
{
    int unused;
} I2C_HandleTypeDef;

typedef enum
{
    HAL_OK = 0x00u,
    HAL_ERROR = 0x01u,
    HAL_BUSY = 0x02u,
    HAL_TIMEOUT = 0x03u
} HAL_StatusTypeDef;

#define I2C_MEMADD_SIZE_8BIT    1u

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c,
                                    uint16_t DevAddress,
                                    uint16_t MemAddress,
                                    uint16_t MemAddSize,
                                    uint8_t *pData,
                                    uint16_t Size,
                                    uint32_t Timeout);

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c,
                                   uint16_t DevAddress,
                                   uint16_t MemAddress,
                                   uint16_t MemAddSize,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout);

void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
#else
#include "stm32h7xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ICM20948_ADDR_AD0_LOW     (0x68u << 1)
#define ICM20948_ADDR_AD0_HIGH    (0x69u << 1)
#define ICM20948_MAG_ADDR         (0x0Cu << 1)

#define ICM20948_DEG_TO_RAD       0.017453292519943295f

typedef struct
{
    float ax_g;
    float ay_g;
    float az_g;
    float gx_dps;
    float gy_dps;
    float gz_dps;
    float mx_uT;
    float my_uT;
    float mz_uT;
    uint8_t mag_data_valid;
} ICM20948_Sample;

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t imu_addr;
    uint8_t mag_addr;
    float accel_lsb_per_g;
    float gyro_lsb_per_dps;
    float mag_lsb_per_uT;
    float accel_bias_g[3];
    float gyro_bias_dps[3];
    float mag_bias_uT[3];
    float mag_scale[3];
    uint8_t mag_axis[3];
    float mag_axis_sign[3];
} ICM20948_HandleTypeDef;

HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *dev,
                                I2C_HandleTypeDef *i2c,
                                uint8_t imu_addr);

HAL_StatusTypeDef ICM20948_Read9Axis(ICM20948_HandleTypeDef *dev,
                                     ICM20948_Sample *sample);

HAL_StatusTypeDef ICM20948_ReadAccelGyro(ICM20948_HandleTypeDef *dev,
                                         ICM20948_Sample *sample);

HAL_StatusTypeDef ICM20948_ReadMag(ICM20948_HandleTypeDef *dev,
                                   ICM20948_Sample *sample);

HAL_StatusTypeDef ICM20948_CalibrateGyro(ICM20948_HandleTypeDef *dev,
                                         uint16_t sample_count,
                                         uint32_t delay_ms);

void ICM20948_SetAccelBiasG(ICM20948_HandleTypeDef *dev,
                            float x_g, float y_g, float z_g);

void ICM20948_SetGyroBiasDps(ICM20948_HandleTypeDef *dev,
                             float x_dps, float y_dps, float z_dps);

void ICM20948_SetMagCalibration(ICM20948_HandleTypeDef *dev,
                                float bias_x_uT, float bias_y_uT, float bias_z_uT,
                                float scale_x, float scale_y, float scale_z);

void ICM20948_SetMagAxisTransform(ICM20948_HandleTypeDef *dev,
                                  uint8_t x_axis, float x_sign,
                                  uint8_t y_axis, float y_sign,
                                  uint8_t z_axis, float z_sign);

#ifdef __cplusplus
}
#endif

#endif
