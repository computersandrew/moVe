#ifndef STM32_HAL_COMPAT_H
#define STM32_HAL_COMPAT_H

#include <stdint.h>

#if defined(MOVE_HOST_BUILD) || defined(ICM20948_HOST_BUILD) || defined(BMP390_HOST_BUILD)
typedef struct
{
    int unused;
} I2C_HandleTypeDef;

typedef struct
{
    int unused;
} UART_HandleTypeDef;

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
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart,
                                      uint8_t *pData,
                                      uint16_t Size);
uint32_t HAL_GetTick(void);
#else
#include "stm32h7xx_hal.h"
#endif

#endif
