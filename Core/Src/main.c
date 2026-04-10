#include "navigation_fusion.h"

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

static NavigationFusion navigation_fusion;
static uint8_t gps_rx_byte;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);
void MX_ADC1_Init(void);
void MX_CRC_Init(void);
void MX_FMC_Init(void);
void MX_DMA2D_Init(void);
void MX_LTDC_Init(void);
void MX_SDMMC2_SD_Init(void);
void MX_USB_OTG_HS_PCD_Init(void);
void Error_Handler(void);

static void NavigationFusion_AppInit(void)
{
    NavigationFusion_Config config;

    config.imu_i2c = &hi2c1;
    config.imu_addr = ICM20948_ADDR_AD0_LOW;
    config.baro_i2c = &hi2c1;
    config.baro_addr = BMP390_ADDR_I2C_PRIM;
    config.sample_frequency_hz = 200.0f;
    config.madgwick_beta = 0.08f;
    config.magnetic_declination_deg = 0.0f;

    if (NavigationFusion_Init(&navigation_fusion, &config) != HAL_OK) {
        Error_Handler();
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    MX_ADC1_Init();
    MX_CRC_Init();
    MX_FMC_Init();
    MX_DMA2D_Init();
    MX_LTDC_Init();
    MX_SDMMC2_SD_Init();
    MX_USB_OTG_HS_PCD_Init();

    NavigationFusion_AppInit();

    if (HAL_UART_Receive_IT(&huart1, &gps_rx_byte, 1u) != HAL_OK) {
        Error_Handler();
    }

    while (1) {
        if (NavigationFusion_Update(&navigation_fusion) != HAL_OK) {
            Error_Handler();
        }

        /*
         * Render from NavigationFusion_GetAircraftDisplay(&navigation_fusion)
         * in a display task or at a fixed display refresh rate.
         */
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        (void)NavigationFusion_ProcessGpsByte(&navigation_fusion, gps_rx_byte);
        (void)HAL_UART_Receive_IT(&huart1, &gps_rx_byte, 1u);
    }
}

void Error_Handler(void)
{
    while (1) {
    }
}
