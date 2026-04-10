#include "icm20948.h"

#include <string.h>

#define ICM20948_BANK_0                    0x00u
#define ICM20948_BANK_2                    0x20u
#define ICM20948_REG_BANK_SEL              0x7Fu

#define ICM20948_REG_WHO_AM_I              0x00u
#define ICM20948_REG_USER_CTRL             0x03u
#define ICM20948_REG_PWR_MGMT_1            0x06u
#define ICM20948_REG_PWR_MGMT_2            0x07u
#define ICM20948_REG_INT_PIN_CFG           0x0Fu
#define ICM20948_REG_ACCEL_XOUT_H          0x2Du

#define ICM20948_REG_GYRO_SMPLRT_DIV       0x00u
#define ICM20948_REG_GYRO_CONFIG_1         0x01u
#define ICM20948_REG_ACCEL_SMPLRT_DIV_1    0x10u
#define ICM20948_REG_ACCEL_SMPLRT_DIV_2    0x11u
#define ICM20948_REG_ACCEL_CONFIG          0x14u

#define ICM20948_WHO_AM_I_VALUE            0xEAu
#define ICM20948_RESET_DEVICE              0x80u
#define ICM20948_CLK_BEST_AVAILABLE        0x01u
#define ICM20948_BYPASS_EN                 0x02u

#define ICM20948_GYRO_DLPF_CFG_51HZ        (3u << 3)
#define ICM20948_GYRO_FS_SEL_2000DPS       (3u << 1)
#define ICM20948_ACCEL_DLPF_CFG_50HZ       (3u << 3)
#define ICM20948_ACCEL_FS_SEL_16G          (3u << 1)

#define AK09916_REG_WIA2                   0x01u
#define AK09916_REG_ST1                    0x10u
#define AK09916_REG_HXL                    0x11u
#define AK09916_REG_ST2                    0x18u
#define AK09916_REG_CNTL2                  0x31u
#define AK09916_REG_CNTL3                  0x32u

#define AK09916_WIA2_VALUE                 0x09u
#define AK09916_SOFT_RESET                 0x01u
#define AK09916_MODE_CONT_100HZ            0x08u
#define AK09916_ST1_DRDY                   0x01u
#define AK09916_ST2_HOFL                   0x08u

#define ICM20948_I2C_TIMEOUT_MS            100u
#define ICM20948_ACCEL_LSB_PER_G_16G       2048.0f
#define ICM20948_GYRO_LSB_PER_DPS_2000     16.4f
#define AK09916_LSB_PER_UT                 6.6666667f

static HAL_StatusTypeDef select_bank(ICM20948_HandleTypeDef *dev, uint8_t bank)
{
    return HAL_I2C_Mem_Write(dev->i2c,
                             dev->imu_addr,
                             ICM20948_REG_BANK_SEL,
                             I2C_MEMADD_SIZE_8BIT,
                             &bank,
                             1u,
                             ICM20948_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef write_reg(ICM20948_HandleTypeDef *dev,
                                   uint8_t bank,
                                   uint8_t reg,
                                   uint8_t value)
{
    HAL_StatusTypeDef status = select_bank(dev, bank);

    if (status != HAL_OK) {
        return status;
    }

    return HAL_I2C_Mem_Write(dev->i2c,
                             dev->imu_addr,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1u,
                             ICM20948_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef read_regs(ICM20948_HandleTypeDef *dev,
                                   uint8_t bank,
                                   uint8_t reg,
                                   uint8_t *data,
                                   uint16_t len)
{
    HAL_StatusTypeDef status = select_bank(dev, bank);

    if (status != HAL_OK) {
        return status;
    }

    return HAL_I2C_Mem_Read(dev->i2c,
                            dev->imu_addr,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            len,
                            ICM20948_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef mag_write_reg(ICM20948_HandleTypeDef *dev,
                                       uint8_t reg,
                                       uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->i2c,
                             dev->mag_addr,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1u,
                             ICM20948_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef mag_read_regs(ICM20948_HandleTypeDef *dev,
                                       uint8_t reg,
                                       uint8_t *data,
                                       uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->i2c,
                            dev->mag_addr,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            len,
                            ICM20948_I2C_TIMEOUT_MS);
}

static int16_t be_i16(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static int16_t le_i16(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[1] << 8) | p[0]);
}

HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *dev,
                                I2C_HandleTypeDef *i2c,
                                uint8_t imu_addr)
{
    uint8_t whoami = 0u;
    HAL_StatusTypeDef status;

    if ((dev == 0) || (i2c == 0)) {
        return HAL_ERROR;
    }

    memset(dev, 0, sizeof(*dev));
    dev->i2c = i2c;
    dev->imu_addr = imu_addr;
    dev->mag_addr = ICM20948_MAG_ADDR;
    dev->accel_lsb_per_g = ICM20948_ACCEL_LSB_PER_G_16G;
    dev->gyro_lsb_per_dps = ICM20948_GYRO_LSB_PER_DPS_2000;
    dev->mag_lsb_per_uT = AK09916_LSB_PER_UT;
    dev->mag_scale[0] = 1.0f;
    dev->mag_scale[1] = 1.0f;
    dev->mag_scale[2] = 1.0f;
    ICM20948_SetMagAxisTransform(dev, 0u, 1.0f, 1u, 1.0f, 2u, 1.0f);

    status = write_reg(dev, ICM20948_BANK_0, ICM20948_REG_PWR_MGMT_1, ICM20948_RESET_DEVICE);
    if (status != HAL_OK) {
        return status;
    }

    HAL_Delay(100u);

    status = write_reg(dev, ICM20948_BANK_0, ICM20948_REG_PWR_MGMT_1, ICM20948_CLK_BEST_AVAILABLE);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, ICM20948_BANK_0, ICM20948_REG_PWR_MGMT_2, 0x00u);
    if (status != HAL_OK) {
        return status;
    }

    status = read_regs(dev, ICM20948_BANK_0, ICM20948_REG_WHO_AM_I, &whoami, 1u);
    if (status != HAL_OK) {
        return status;
    }

    if (whoami != ICM20948_WHO_AM_I_VALUE) {
        return HAL_ERROR;
    }

    status = write_reg(dev,
                       ICM20948_BANK_2,
                       ICM20948_REG_GYRO_CONFIG_1,
                       ICM20948_GYRO_DLPF_CFG_51HZ | ICM20948_GYRO_FS_SEL_2000DPS);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, ICM20948_BANK_2, ICM20948_REG_GYRO_SMPLRT_DIV, 0x00u);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev,
                       ICM20948_BANK_2,
                       ICM20948_REG_ACCEL_CONFIG,
                       ICM20948_ACCEL_DLPF_CFG_50HZ | ICM20948_ACCEL_FS_SEL_16G);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, ICM20948_BANK_2, ICM20948_REG_ACCEL_SMPLRT_DIV_1, 0x00u);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, ICM20948_BANK_2, ICM20948_REG_ACCEL_SMPLRT_DIV_2, 0x04u);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, ICM20948_BANK_0, ICM20948_REG_USER_CTRL, 0x00u);
    if (status != HAL_OK) {
        return status;
    }

    status = write_reg(dev, ICM20948_BANK_0, ICM20948_REG_INT_PIN_CFG, ICM20948_BYPASS_EN);
    if (status != HAL_OK) {
        return status;
    }

    HAL_Delay(10u);

    status = mag_read_regs(dev, AK09916_REG_WIA2, &whoami, 1u);
    if (status != HAL_OK) {
        return status;
    }

    if (whoami != AK09916_WIA2_VALUE) {
        return HAL_ERROR;
    }

    status = mag_write_reg(dev, AK09916_REG_CNTL3, AK09916_SOFT_RESET);
    if (status != HAL_OK) {
        return status;
    }

    HAL_Delay(10u);

    return mag_write_reg(dev, AK09916_REG_CNTL2, AK09916_MODE_CONT_100HZ);
}

HAL_StatusTypeDef ICM20948_ReadAccelGyro(ICM20948_HandleTypeDef *dev,
                                         ICM20948_Sample *sample)
{
    uint8_t data[12];
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    HAL_StatusTypeDef status;

    if ((dev == 0) || (sample == 0)) {
        return HAL_ERROR;
    }

    status = read_regs(dev, ICM20948_BANK_0, ICM20948_REG_ACCEL_XOUT_H, data, sizeof(data));
    if (status != HAL_OK) {
        return status;
    }

    ax = be_i16(&data[0]);
    ay = be_i16(&data[2]);
    az = be_i16(&data[4]);
    gx = be_i16(&data[6]);
    gy = be_i16(&data[8]);
    gz = be_i16(&data[10]);

    sample->ax_g = ((float)ax / dev->accel_lsb_per_g) - dev->accel_bias_g[0];
    sample->ay_g = ((float)ay / dev->accel_lsb_per_g) - dev->accel_bias_g[1];
    sample->az_g = ((float)az / dev->accel_lsb_per_g) - dev->accel_bias_g[2];
    sample->gx_dps = ((float)gx / dev->gyro_lsb_per_dps) - dev->gyro_bias_dps[0];
    sample->gy_dps = ((float)gy / dev->gyro_lsb_per_dps) - dev->gyro_bias_dps[1];
    sample->gz_dps = ((float)gz / dev->gyro_lsb_per_dps) - dev->gyro_bias_dps[2];

    return HAL_OK;
}

HAL_StatusTypeDef ICM20948_ReadMag(ICM20948_HandleTypeDef *dev,
                                   ICM20948_Sample *sample)
{
    uint8_t status1 = 0u;
    uint8_t data[8];
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    float raw_uT[3];
    float aligned_uT[3];
    HAL_StatusTypeDef status;

    if ((dev == 0) || (sample == 0)) {
        return HAL_ERROR;
    }

    sample->mag_data_valid = 0u;

    status = mag_read_regs(dev, AK09916_REG_ST1, &status1, 1u);
    if (status != HAL_OK) {
        return status;
    }

    if ((status1 & AK09916_ST1_DRDY) == 0u) {
        return HAL_OK;
    }

    status = mag_read_regs(dev, AK09916_REG_HXL, data, sizeof(data));
    if (status != HAL_OK) {
        return status;
    }

    if ((data[7] & AK09916_ST2_HOFL) != 0u) {
        return HAL_ERROR;
    }

    raw_x = le_i16(&data[0]);
    raw_y = le_i16(&data[2]);
    raw_z = le_i16(&data[4]);

    raw_uT[0] = (float)raw_x / dev->mag_lsb_per_uT;
    raw_uT[1] = (float)raw_y / dev->mag_lsb_per_uT;
    raw_uT[2] = (float)raw_z / dev->mag_lsb_per_uT;

    aligned_uT[0] = raw_uT[dev->mag_axis[0]] * dev->mag_axis_sign[0];
    aligned_uT[1] = raw_uT[dev->mag_axis[1]] * dev->mag_axis_sign[1];
    aligned_uT[2] = raw_uT[dev->mag_axis[2]] * dev->mag_axis_sign[2];

    sample->mx_uT = (aligned_uT[0] - dev->mag_bias_uT[0]) * dev->mag_scale[0];
    sample->my_uT = (aligned_uT[1] - dev->mag_bias_uT[1]) * dev->mag_scale[1];
    sample->mz_uT = (aligned_uT[2] - dev->mag_bias_uT[2]) * dev->mag_scale[2];
    sample->mag_data_valid = 1u;

    return HAL_OK;
}

HAL_StatusTypeDef ICM20948_Read9Axis(ICM20948_HandleTypeDef *dev,
                                     ICM20948_Sample *sample)
{
    HAL_StatusTypeDef status;

    if ((dev == 0) || (sample == 0)) {
        return HAL_ERROR;
    }

    status = ICM20948_ReadAccelGyro(dev, sample);
    if (status != HAL_OK) {
        return status;
    }

    return ICM20948_ReadMag(dev, sample);
}

HAL_StatusTypeDef ICM20948_CalibrateGyro(ICM20948_HandleTypeDef *dev,
                                         uint16_t sample_count,
                                         uint32_t delay_ms)
{
    ICM20948_Sample sample;
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_z = 0.0f;
    uint16_t i;
    HAL_StatusTypeDef status;

    if ((dev == 0) || (sample_count == 0u)) {
        return HAL_ERROR;
    }

    ICM20948_SetGyroBiasDps(dev, 0.0f, 0.0f, 0.0f);

    for (i = 0u; i < sample_count; i++) {
        status = ICM20948_ReadAccelGyro(dev, &sample);
        if (status != HAL_OK) {
            return status;
        }

        sum_x += sample.gx_dps;
        sum_y += sample.gy_dps;
        sum_z += sample.gz_dps;
        HAL_Delay(delay_ms);
    }

    ICM20948_SetGyroBiasDps(dev,
                            sum_x / (float)sample_count,
                            sum_y / (float)sample_count,
                            sum_z / (float)sample_count);

    return HAL_OK;
}

void ICM20948_SetAccelBiasG(ICM20948_HandleTypeDef *dev,
                            float x_g, float y_g, float z_g)
{
    if (dev == 0) {
        return;
    }

    dev->accel_bias_g[0] = x_g;
    dev->accel_bias_g[1] = y_g;
    dev->accel_bias_g[2] = z_g;
}

void ICM20948_SetGyroBiasDps(ICM20948_HandleTypeDef *dev,
                             float x_dps, float y_dps, float z_dps)
{
    if (dev == 0) {
        return;
    }

    dev->gyro_bias_dps[0] = x_dps;
    dev->gyro_bias_dps[1] = y_dps;
    dev->gyro_bias_dps[2] = z_dps;
}

void ICM20948_SetMagCalibration(ICM20948_HandleTypeDef *dev,
                                float bias_x_uT, float bias_y_uT, float bias_z_uT,
                                float scale_x, float scale_y, float scale_z)
{
    if (dev == 0) {
        return;
    }

    dev->mag_bias_uT[0] = bias_x_uT;
    dev->mag_bias_uT[1] = bias_y_uT;
    dev->mag_bias_uT[2] = bias_z_uT;
    dev->mag_scale[0] = (scale_x > 0.0f) ? scale_x : 1.0f;
    dev->mag_scale[1] = (scale_y > 0.0f) ? scale_y : 1.0f;
    dev->mag_scale[2] = (scale_z > 0.0f) ? scale_z : 1.0f;
}

void ICM20948_SetMagAxisTransform(ICM20948_HandleTypeDef *dev,
                                  uint8_t x_axis, float x_sign,
                                  uint8_t y_axis, float y_sign,
                                  uint8_t z_axis, float z_sign)
{
    if (dev == 0) {
        return;
    }

    dev->mag_axis[0] = (x_axis < 3u) ? x_axis : 0u;
    dev->mag_axis[1] = (y_axis < 3u) ? y_axis : 1u;
    dev->mag_axis[2] = (z_axis < 3u) ? z_axis : 2u;
    dev->mag_axis_sign[0] = (x_sign < 0.0f) ? -1.0f : 1.0f;
    dev->mag_axis_sign[1] = (y_sign < 0.0f) ? -1.0f : 1.0f;
    dev->mag_axis_sign[2] = (z_sign < 0.0f) ? -1.0f : 1.0f;
}
