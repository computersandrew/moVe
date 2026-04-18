# moVe Custom Board Pinout Draft

Target MCU: `STM32H723ZGT6` / CubeMX device `STM32H723ZGTx`

Board: custom KiCad single-board design

This is the current pin/peripheral plan from `moVe.ioc`. CubeMX should remain the source of truth for the final LTDC, FMC SDRAM, and SDMMC2 pins because those high-pin-count peripherals compete for package pins.

## Peripheral Summary

| Function | STM32 Peripheral | Current Plan | Notes |
| --- | --- | --- | --- |
| ICM-20948 + BMP390 | `I2C1` | `PB6` SCL, `PB7` SDA | Use correct pull-up voltage for the bus domain. |
| MAX-M10S GNSS | `USART1` | `PB14` TX, `PB15` RX | Asynchronous UART, 8N1. STM32 TX to GPS RX, GPS TX to STM32 RX. |
| NORA-B261 Bluetooth | `USART3` | `PC10` TX, `PB11` RX | Asynchronous UART, 8N1 AT-command host link. Reserve RTS/CTS if high-rate telemetry needs flow control. |
| NORA-B261 control | GPIO | TBD | Add reset, recovery/test access, and optional IRQ/attention GPIOs. |
| SD card | `SDMMC2` | CubeMX-selected 4-bit bus | Use `SD 4 bits Wide bus`; do not enable auto-direction voltage converter unless a matching external translator is added. |
| Display | `LTDC` | RGB TFT interface | Newhaven `NHD-5.0-800480TF-ATXL-CTP`, 800x480, parallel RGB, I2C capacitive touch. |
| Graphics acceleration | `DMA2D` | Enable | Use for framebuffer fills, blits, and pixel format conversion. |
| Framebuffer memory | `FMC` | `SDRAM 1`, `SDCKE0 + SDNE0`, 16-bit data | Choose the final SDRAM IC before locking row/column/timing values. |
| Chart/log storage | `SDMMC2` + FatFs | Enable later | Store chart/map tiles and logs on microSD. |
| Battery monitor | `ADC1` | `PC4 / ADC1_INP4` | Use a resistor divider; do not feed pack voltage directly into the MCU. |
| Aircraft CO sensor | ADC + GPIO/PWM | TBD | MQ-7 analog output plus heater-control hardware for high/low heater cycle. |
| USB-C device/power | `USB_OTG_HS` | Device-only, embedded FS PHY if no ULPI PHY | Use USB-C VBUS for bring-up power through protection and regulators. VBUS sensing can be disabled in CubeMX for now. |
| Debug | `SWD` | `PA13` SWDIO, `PA14` SWCLK | Add `NRST`, 3V3, GND, and optional `PB3` SWO. |

## CubeMX Starting Choices

| Peripheral | Setting |
| --- | --- |
| Core | Single Cortex-M7 only; there is no M4 ownership split on STM32H723. |
| `DEBUG` / `SYS` | `Serial Wire`. |
| `PWR Wake-Up 1` | Disabled for first bring-up. |
| `USB_OTG_HS` | `Device_Only`; use embedded FS PHY unless adding an external ULPI PHY. |
| `FMC` | `SDRAM 1`, `SDCKE0 + SDNE0`, 16-bit data, byte enable on if the SDRAM has `LDQM/UDQM`. |
| `SDMMC2` | `SD 4 bits Wide bus`, no auto-direction voltage converter. |
| `LTDC` | Parallel RGB TFT output for the Newhaven display. |
| `DMA2D` | Enabled for GUI acceleration. |
| `FreeRTOS` | Disabled for first hardware bring-up; add after peripherals are proven. |
| `CRC` | Enable. |
| `RNG` | Optional; useful for security/pairing tokens if STM32 handles any. |
| `DFSDM1`, `I2S`, `SAI`, `DCMI` | Disabled unless audio/camera features are added later. |

## Sensor Bus

Use one shared I2C bus for the pressure and inertial sensors:

| Device | Signal | STM32 Pin | Notes |
| --- | --- | --- | --- |
| ICM-20948 | SCL | `PB6 / I2C1_SCL` | Check voltage domain. |
| ICM-20948 | SDA | `PB7 / I2C1_SDA` | Check voltage domain. |
| BMP390 | SCL | `PB6 / I2C1_SCL` | Same bus as IMU. |
| BMP390 | SDA | `PB7 / I2C1_SDA` | Same bus as IMU. |
| ICM-20948 | INT | GPIO TBD | Optional data-ready interrupt. |
| BMP390 | INT | GPIO TBD | Optional data-ready interrupt. |

Address plan:

| Device | Address |
| --- | --- |
| ICM-20948 | `0x68` default, `0x69` if AD0 high |
| BMP390 | `0x76` default, `0x77` if SDO high |

Power note: if the ICM-20948 is kept on a 1.8 V bus while the BMP390 is on 3.3 V, do not place them on the same I2C segment without level shifting. A cleaner design is either all sensor I2C at one compatible voltage or separate I2C buses per voltage domain.

## GNSS

MAX-M10S is handled as a UART byte stream. The parser supports UBX binary and NMEA.

| MAX-M10S Signal | STM32 Pin | Notes |
| --- | --- | --- |
| TXD | `PB15 / USART1_RX` | GPS to STM32. |
| RXD | `PB14 / USART1_TX` | STM32 to GPS for UBX config. |
| TIMEPULSE | GPIO TBD | Optional PPS/timepulse input. |
| EXTINT | GPIO TBD | Optional wake/interrupt. |
| RESET_N | GPIO TBD | Optional reset control. |

Hardware notes:

- Include a u.FL connector and/or ceramic patch antenna footprint.
- Follow u-blox RF layout guidance for antenna trace, keepout, and ground.
- Add backup power if retaining GNSS time/config is desired.

## Bluetooth Co-Processor

Use a u-blox NORA-B261 Bluetooth LE module on the board. The NORA-B261 is the u-connectXpress variant with an antenna pin, so the STM32 should treat it as an AT-command Bluetooth co-processor over UART. Initial host link is asynchronous UART because it is simple and maps cleanly into STM32 firmware. SPI is not needed for first bring-up.

| NORA-B261 Signal | STM32 Pin | Notes |
| --- | --- | --- |
| NORA UART RX | `PC10 / USART3_TX` | STM32 to NORA-B261. |
| NORA UART TX | `PB11 / USART3_RX` | NORA-B261 to STM32. |
| NORA RTS | GPIO TBD | Optional flow control; reserve if routing allows. |
| NORA CTS | GPIO TBD | Optional flow control; reserve if routing allows. |
| NORA RESET_N | GPIO TBD | Strongly recommended. |
| NORA recovery/test | GPIO or test pad TBD | Recommended for firmware update/recovery access. |
| NORA IRQ/attention | GPIO TBD | Optional host wake/attention line if used by the host protocol. |
| NORA ANT | 2.4 GHz RF path | Route as 50 ohm controlled impedance to chip antenna or u.FL with matching network. |

NORA-B261 hardware notes:

- Use the u-blox module land pattern and keepout guidance instead of a bare-radio RF reference layout.
- Route the antenna pin through a short 50 ohm RF path with matching network access.
- Decide chip antenna or u.FL before PCB layout.
- Consider UART hardware flow control if high-rate telemetry will be streamed.
- Expose reset and recovery/test access for bring-up and module firmware recovery.

## USB-C

Use USB-C as a device connection and interim 5 V board power source.

| USB-C Signal | STM32 / Board Connection | Notes |
| --- | --- | --- |
| D- | `PA11 / USB_OTG_HS_DM` | Use the embedded FS PHY path unless adding ULPI. Route as a USB differential pair. |
| D+ | `PA12 / USB_OTG_HS_DP` | Use the embedded FS PHY path unless adding ULPI. Route as a USB differential pair. |
| CC1/CC2 | `5.1 kOhm` pulldowns to GND | Required for USB-C sink/device attach behavior. |
| VBUS | Power input path | Route through fuse/eFuse/protection into the regulator tree. |
| VBUS sense | GPIO/ADC/USB sense pin TBD | Optional for now; do not connect 5 V directly to a random GPIO. |

CubeMX can leave VBUS sensing disabled during first bring-up if the board is powered from the same USB connector. Add a resistor divider, jumper, or test option if firmware should detect USB presence later.

## Battery And Power

The first board can power from USB-C VBUS. If adding a two-cell lithium pack later, do not connect pack voltage directly to the STM32.

| Function | Plan |
| --- | --- |
| USB input | USB-C VBUS through protection into the regulator tree. |
| Battery input | Future 2S lithium/LiFePO4-compatible path with power mux/ideal diode behavior. |
| Battery voltage monitor | Resistor divider into `PC4 / ADC1_INP4`. |
| Current monitor | Optional shunt/current-sense amplifier. |
| Backlight supply | Separate LED boost/current driver for the Newhaven backlight. |
| 3.3 V rail | STM32, GNSS, NORA-B261, BMP390, touch, SD, and display logic. |
| 1.8 V rail | ICM-20948 if kept at 1.8 V. |

## Aircraft Carbon Monoxide Sensor

Use an MQ-7/MQ-7B only for the aircraft build variant. The sensor needs a heater driver that can provide the high-temperature clean phase and low-temperature measurement phase, plus an analog output routed to an ADC channel.

| MQ-7 Function | STM32 / Board Connection | Notes |
| --- | --- | --- |
| Analog output | ADC TBD | Add input scaling/filtering so the ADC never exceeds the STM32 analog range. |
| Heater high/low drive | GPIO/PWM + external driver TBD | Do not drive the heater directly from an STM32 GPIO. |
| Sensor supply/load resistor | Analog front end TBD | Select load resistor and calibration path during schematic work. |

Firmware thresholds are 25 ppm, 50 ppm, and 100 ppm; real ppm accuracy depends on heater timing, calibration, temperature/humidity behavior, and sensor placement.

## SD Card

Use `SDMMC2` because that is the SDMMC instance CubeMX exposed cleanly for the STM32H723ZGT6 setup.

| SD Signal | STM32 Pin | Notes |
| --- | --- | --- |
| D0 | `PG9 / SDMMC2_D0` | 4-bit SDMMC. |
| D1 | `PG10 / SDMMC2_D1` | 4-bit SDMMC. |
| D2 | `PG11 / SDMMC2_D2` | 4-bit SDMMC. |
| D3 | `PG12 / SDMMC2_D3` | 4-bit SDMMC. |
| CK | `PD6 / SDMMC2_CK` | Controlled impedance/routing care. |
| CMD | `PA0 / SDMMC2_CMD` | Pull-up per SD requirements. |
| CD | GPIO TBD | Optional card detect. |

Do not use the auto-direction voltage converter mode unless the schematic includes the matching external SD voltage translator.

## Display

Current display direction: Newhaven `NHD-5.0-800480TF-ATXL-CTP`.

| Display Signal | STM32 / Board Connection | Notes |
| --- | --- | --- |
| `R0` | `PG13 / LTDC_R0` | 24-bit RGB888 bus from CubeMX. |
| `R1` | `PA2 / LTDC_R1` | 24-bit RGB888 bus from CubeMX. |
| `R2` | `PA1 / LTDC_R2` | 24-bit RGB888 bus from CubeMX. |
| `R3` | `PB0 / LTDC_R3` | 24-bit RGB888 bus from CubeMX. |
| `R4` | `PA5 / LTDC_R4` | 24-bit RGB888 bus from CubeMX. |
| `R5` | `PC0 / LTDC_R5` | 24-bit RGB888 bus from CubeMX. |
| `R6` | `PB1 / LTDC_R6` | 24-bit RGB888 bus from CubeMX. |
| `R7` | `PG6 / LTDC_R7` | 24-bit RGB888 bus from CubeMX. |
| `G0` | `PE5 / LTDC_G0` | 24-bit RGB888 bus from CubeMX. |
| `G1` | `PE6 / LTDC_G1` | 24-bit RGB888 bus from CubeMX. |
| `G2` | `PA6 / LTDC_G2` | 24-bit RGB888 bus from CubeMX. |
| `G3` | `PC9 / LTDC_G3` | 24-bit RGB888 bus from CubeMX. |
| `G4` | `PB10 / LTDC_G4` | 24-bit RGB888 bus from CubeMX. |
| `G5` | `PC1 / LTDC_G5` | 24-bit RGB888 bus from CubeMX. |
| `G6` | `PC7 / LTDC_G6` | 24-bit RGB888 bus from CubeMX. |
| `G7` | `PD3 / LTDC_G7` | 24-bit RGB888 bus from CubeMX. |
| `B0` | `PE4 / LTDC_B0` | 24-bit RGB888 bus from CubeMX. |
| `B1` | `PA10 / LTDC_B1` | 24-bit RGB888 bus from CubeMX. |
| `B2` | `PA3 / LTDC_B2` | 24-bit RGB888 bus from CubeMX. |
| `B3` | `PA8 / LTDC_B3` | 24-bit RGB888 bus from CubeMX. |
| `B4` | `PC11 / LTDC_B4` | 24-bit RGB888 bus from CubeMX. |
| `B5` | `PB5 / LTDC_B5` | 24-bit RGB888 bus from CubeMX. |
| `B6` | `PA15 / LTDC_B6` | 24-bit RGB888 bus from CubeMX. |
| `B7` | `PD2 / LTDC_B7` | 24-bit RGB888 bus from CubeMX. |
| `CLKIN` | `PG7 / LTDC_CLK` | Pixel clock. |
| `HSD` | `PC6 / LTDC_HSYNC` | Horizontal sync. |
| `VSD` | `PA4 / LTDC_VSYNC` | Vertical sync. |
| `DEN` | `PF10 / LTDC_DE` | Display enable. |
| `STBYB` | GPIO TBD | Display standby control. |
| `LED+ / LED-` | Backlight LED driver | Do not drive directly from STM32. |
| Touch SCL/SDA | I2C bus TBD | Capacitive touch interface. |
| Touch `/INT` | GPIO EXTI TBD | Touch interrupt. |
| Touch `/RESET` | GPIO output TBD | Touch reset. |

The physical panel interface is 24-bit RGB, but the first framebuffer should use `RGB565` in SDRAM to reduce memory bandwidth and capacity pressure.

## External SDRAM

Use FMC `SDRAM 1` with `SDCKE0 + SDNE0` for the first board spin.

| SDRAM Signal Group | Plan |
| --- | --- |
| Data | `PD14` D0, `PD15` D1, `PD0` D2, `PD1` D3, `PE7` D4, `PE8` D5, `PE9` D6, `PE10` D7, `PE11` D8, `PE12` D9, `PE13` D10, `PE14` D11, `PE15` D12, `PD8` D13, `PD9` D14, `PD10` D15. |
| Byte masks | `PE0 / FMC_NBL0`, `PE1 / FMC_NBL1`. |
| Address | `PF0` A0, `PF1` A1, `PF2` A2, `PF3` A3, `PF4` A4, `PF5` A5, `PF12` A6, `PF13` A7, `PF14` A8, `PF15` A9, `PG0` A10, `PG1` A11, `PG2` A12. |
| Bank address | `PG4 / FMC_BA0`, `PG5 / FMC_BA1`. |
| Clock/chip enable | `PG8 / FMC_SDCLK`, `PC3_C / FMC_SDCKE0`, `PC2_C / FMC_SDNE0`. |
| Control | `PF11 / FMC_SDNRAS`, `PG15 / FMC_SDNCAS`, `PA7 / FMC_SDNWE`. |
| Timing | Copy from the selected SDRAM datasheet into CubeMX. |

Pick the exact SDRAM part before finalizing row bits, column bits, CAS latency, refresh, and timing values.

## Debug And Boot

| Function | STM32 Pin | Notes |
| --- | --- | --- |
| SWDIO | `PA13` | STM32 debug header. |
| SWCLK | `PA14` | STM32 debug header. |
| SWO | `PB3` optional | Trace output if it does not conflict with SDMMC2 routing. |
| NRST | `NRST` | STM32 reset. |
| BOOT0 | `BOOT0` | Add pulldown and optional boot strap/test point. |

Expose NORA-B261 reset/recovery test access. A separate BLE SWD header is not part of the u-connectXpress module bring-up path.

## Open Pin Decisions

- Final LTDC timing values for the Newhaven panel.
- Exact SDRAM part number and timing values.
- NORA-B261 GPIO choices for reset, recovery/test, IRQ, RTS, and CTS.
- GPS TIMEPULSE and reset pins.
- IMU and BMP390 interrupt pins.
- MQ-7 ADC and heater-control pins for aircraft variant only.
- SD card detect pin.
- USB VBUS sense option.
- Power tree and voltage domains for sensor I2C.
