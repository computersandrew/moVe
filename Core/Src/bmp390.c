#include "bmp390.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define BMP390_REG_CHIP_ID              0x00u
#define BMP390_REG_SENS_STATUS          0x03u
#define BMP390_REG_DATA                 0x04u
#define BMP390_REG_PWR_CTRL             0x1Bu
#define BMP390_REG_OSR                  0x1Cu
#define BMP390_REG_ODR                  0x1Du
#define BMP390_REG_CONFIG               0x1Fu
#define BMP390_REG_CALIB_DATA           0x31u
#define BMP390_REG_CMD                  0x7Eu

#define BMP390_CHIP_ID                  0x60u
#define BMP390_SOFT_RESET               0xB6u
#define BMP390_PRESS_EN                 0x01u
#define BMP390_TEMP_EN                  0x02u
#define BMP390_MODE_SLEEP               0x00u
#define BMP390_MODE_FORCED              0x10u
#define BMP390_MODE_NORMAL              0x30u
#define BMP390_STATUS_DRDY_PRESS        0x20u
#define BMP390_STATUS_DRDY_TEMP         0x40u

#define BMP390_PRESS_OS_8X              0x03u
#define BMP390_TEMP_OS_2X               (0x01u << 3)
#define BMP390_ODR_50HZ                 0x02u
#define BMP390_IIR_COEFF_7              (0x03u << 1)
#define BMP390_I2C_TIMEOUT_MS           100u
#define BMP390_CALIB_LEN                21u
#define BMP390_DATA_LEN                 6u
#define BMP390_MIN_PRESSURE_PA          30000.0f
#define BMP390_MAX_PRESSURE_PA          125000.0f
#define BMP390_MIN_SEA_LEVEL_PA         80000.0f
#define BMP390_MAX_SEA_LEVEL_PA         120000.0f

static HAL_StatusTypeDef write_reg(BMP390_HandleTypeDef *dev,
                                   uint8_t reg,
                                   uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->i2c,
                             dev->addr,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1u,
                             BMP390_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef read_regs(BMP390_HandleTypeDef *dev,
                                   uint8_t reg,
                                   uint8_t *data,
                                   uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->i2c,
                            dev->addr,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            len,
                            BMP390_I2C_TIMEOUT_MS);
}

static uint16_t u16_le(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
}

static int16_t i16_le(const uint8_t *data)
{
    return (int16_t)u16_le(data);
}

static uint32_t u24_le(const uint8_t *data)
{
    return ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
}

static void parse_calibration(BMP390_HandleTypeDef *dev, const uint8_t *data)
{
    BMP390_Calibration *calib = &dev->calib;

    calib->par_t1 = (double)u16_le(&data[0]) / 0.00390625;
    calib->par_t2 = (double)u16_le(&data[2]) / 1073741824.0;
    calib->par_t3 = (double)((int8_t)data[4]) / 281474976710656.0;
    calib->par_p1 = (double)(i16_le(&data[5]) - 16384) / 1048576.0;
    calib->par_p2 = (double)(i16_le(&data[7]) - 16384) / 536870912.0;
    calib->par_p3 = (double)((int8_t)data[9]) / 4294967296.0;
    calib->par_p4 = (double)((int8_t)data[10]) / 137438953472.0;
    calib->par_p5 = (double)u16_le(&data[11]) / 0.125;
    calib->par_p6 = (double)u16_le(&data[13]) / 64.0;
    calib->par_p7 = (double)((int8_t)data[15]) / 256.0;
    calib->par_p8 = (double)((int8_t)data[16]) / 32768.0;
    calib->par_p9 = (double)i16_le(&data[17]) / 281474976710656.0;
    calib->par_p10 = (double)((int8_t)data[19]) / 281474976710656.0;
    calib->par_p11 = (double)((int8_t)data[20]) / 36893488147419103232.0;
}

static double compensate_temperature(BMP390_HandleTypeDef *dev, uint32_t raw_temp)
{
    BMP390_Calibration *calib = &dev->calib;
    const double partial1 = (double)raw_temp - calib->par_t1;
    const double partial2 = partial1 * calib->par_t2;

    calib->t_lin = partial2 + ((partial1 * partial1) * calib->par_t3);

    return calib->t_lin;
}

static double compensate_pressure(const BMP390_HandleTypeDef *dev, uint32_t raw_pressure)
{
    const BMP390_Calibration *calib = &dev->calib;
    const double pressure = (double)raw_pressure;
    const double t = calib->t_lin;
    double partial1;
    double partial2;
    double partial3;
    double partial4;
    double out1;
    double out2;
    double compensated;

    partial1 = calib->par_p6 * t;
    partial2 = calib->par_p7 * t * t;
    partial3 = calib->par_p8 * t * t * t;
    out1 = calib->par_p5 + partial1 + partial2 + partial3;

    partial1 = calib->par_p2 * t;
    partial2 = calib->par_p3 * t * t;
    partial3 = calib->par_p4 * t * t * t;
    out2 = pressure * (calib->par_p1 + partial1 + partial2 + partial3);

    partial1 = pressure * pressure;
    partial2 = calib->par_p9 + (calib->par_p10 * t);
    partial3 = partial1 * partial2;
    partial4 = partial3 + (pressure * pressure * pressure * calib->par_p11);
    compensated = out1 + out2 + partial4;

    if (compensated < BMP390_MIN_PRESSURE_PA) {
        compensated = BMP390_MIN_PRESSURE_PA;
    } else if (compensated > BMP390_MAX_PRESSURE_PA) {
        compensated = BMP390_MAX_PRESSURE_PA;
    }

    return compensated;
}

HAL_StatusTypeDef BMP390_Init(BMP390_HandleTypeDef *dev,
                              I2C_HandleTypeDef *i2c,
                              uint8_t addr)
{
    uint8_t chip_id = 0u;
    uint8_t calib_data[BMP390_CALIB_LEN];
    HAL_StatusTypeDef status;

    if ((dev == NULL) || (i2c == NULL)) {
        return HAL_ERROR;
    }

    memset(dev, 0, sizeof(*dev));
    dev->i2c = i2c;
    dev->addr = addr;
    dev->sea_level_pressure_pa = BMP390_SEA_LEVEL_PRESSURE_PA;

    status = write_reg(dev, BMP390_REG_CMD, BMP390_SOFT_RESET);
    if (status != HAL_OK) {
        return status;
    }

    HAL_Delay(10u);

    status = read_regs(dev, BMP390_REG_CHIP_ID, &chip_id, 1u);
    if (status != HAL_OK) {
        return status;
    }

    if (chip_id != BMP390_CHIP_ID) {
        return HAL_ERROR;
    }

    status = read_regs(dev, BMP390_REG_CALIB_DATA, calib_data, BMP390_CALIB_LEN);
    if (status != HAL_OK) {
        return status;
    }
    parse_calibration(dev, calib_data);

    status = write_reg(dev, BMP390_REG_PWR_CTRL, BMP390_MODE_SLEEP);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, BMP390_REG_OSR, BMP390_PRESS_OS_8X | BMP390_TEMP_OS_2X);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, BMP390_REG_ODR, BMP390_ODR_50HZ);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, BMP390_REG_CONFIG, BMP390_IIR_COEFF_7);
    if (status != HAL_OK) {
        return status;
    }

    return write_reg(dev,
                     BMP390_REG_PWR_CTRL,
                     BMP390_PRESS_EN | BMP390_TEMP_EN | BMP390_MODE_NORMAL);
}

HAL_StatusTypeDef BMP390_Read(BMP390_HandleTypeDef *dev,
                              BMP390_Sample *sample)
{
    uint8_t status_reg = 0u;
    uint8_t data[BMP390_DATA_LEN];
    uint32_t raw_pressure;
    uint32_t raw_temperature;
    double temperature_c;
    double pressure_pa;
    HAL_StatusTypeDef status;

    if ((dev == NULL) || (sample == NULL)) {
        return HAL_ERROR;
    }

    sample->data_ready = 0u;

    status = read_regs(dev, BMP390_REG_SENS_STATUS, &status_reg, 1u);
    if (status != HAL_OK) {
        return status;
    }

    if ((status_reg & (BMP390_STATUS_DRDY_PRESS | BMP390_STATUS_DRDY_TEMP)) !=
        (BMP390_STATUS_DRDY_PRESS | BMP390_STATUS_DRDY_TEMP)) {
        return HAL_OK;
    }

    status = read_regs(dev, BMP390_REG_DATA, data, BMP390_DATA_LEN);
    if (status != HAL_OK) {
        return status;
    }

    raw_pressure = u24_le(&data[0]);
    raw_temperature = u24_le(&data[3]);
    temperature_c = compensate_temperature(dev, raw_temperature);
    pressure_pa = compensate_pressure(dev, raw_pressure);

    sample->pressure_pa = (float)pressure_pa;
    sample->temperature_c = (float)temperature_c;
    sample->altitude_m = BMP390_PressureToAltitudeM(sample->pressure_pa,
                                                    dev->sea_level_pressure_pa);
    sample->data_ready = 1u;

    return HAL_OK;
}

HAL_StatusTypeDef BMP390_ReadForced(BMP390_HandleTypeDef *dev,
                                    BMP390_Sample *sample)
{
    HAL_StatusTypeDef status;
    uint8_t tries;

    if ((dev == NULL) || (sample == NULL)) {
        return HAL_ERROR;
    }

    status = write_reg(dev,
                       BMP390_REG_PWR_CTRL,
                       BMP390_PRESS_EN | BMP390_TEMP_EN | BMP390_MODE_FORCED);
    if (status != HAL_OK) {
        return status;
    }

    for (tries = 0u; tries < 10u; tries++) {
        HAL_Delay(5u);
        status = BMP390_Read(dev, sample);
        if ((status != HAL_OK) || (sample->data_ready != 0u)) {
            return status;
        }
    }

    return HAL_TIMEOUT;
}

void BMP390_SetSeaLevelPressure(BMP390_HandleTypeDef *dev,
                                float sea_level_pressure_pa)
{
    if (dev == NULL) {
        return;
    }

    if ((sea_level_pressure_pa >= BMP390_MIN_SEA_LEVEL_PA) &&
        (sea_level_pressure_pa <= BMP390_MAX_SEA_LEVEL_PA)) {
        dev->sea_level_pressure_pa = sea_level_pressure_pa;
    }
}

float BMP390_PressureToAltitudeM(float pressure_pa,
                                 float sea_level_pressure_pa)
{
    if ((pressure_pa <= 0.0f) || (sea_level_pressure_pa <= 0.0f)) {
        return 0.0f;
    }

    return 44330.0f * (1.0f - powf(pressure_pa / sea_level_pressure_pa, 0.190294957f));
}
