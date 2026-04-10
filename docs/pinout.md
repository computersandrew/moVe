# moVe Custom Board Pinout Draft

Target MCU: `STM32H723ZGT6` / CubeMX device `STM32H723ZGTx`

Board: custom KiCad single-board design

This is the current first-pass pin/peripheral plan. CubeMX should remain the source of truth for the final LTDC, FMC SDRAM, and SDMMC2 pins because those high-pin-count peripherals compete for package pins.

## Peripheral Summary

| Function | STM32 Peripheral | Current Plan | Notes |
| --- | --- | --- | --- |
| ICM-20948 + BMP390 | `I2C1` | `PB8` SCL, `PB9` SDA if no LTDC conflict | Use correct pull-up voltage for the bus domain. Move to another I2C bus if full RGB888 LTDC needs these pins. |
| MAX-M10S GNSS | `USART1` | `PA9` TX, `PA10` RX if no LTDC/USB conflict | Asynchronous UART, 8N1. STM32 TX to GPS RX, GPS TX to STM32 RX. |
| nRF52840 Bluetooth | `USART3` | `PD8` TX, `PD9` RX or CubeMX-selected alternate pins | Asynchronous UART, 8N1. Reserve RTS/CTS if high-rate telemetry needs flow control. |
| nRF52840 control | GPIO | TBD | Add `NRF_RESET_N`, `NRF_BOOT_DFU`, and optional `NRF_IRQ`. |
| SD card | `SDMMC2` | CubeMX-selected 4-bit bus | Use `SD 4 bits Wide bus`; do not enable auto-direction voltage converter unless a matching external translator is added. |
| Display | `LTDC` | RGB TFT interface | Newhaven `NHD-5.0-800480TF-ATXL-CTP`, 800x480, parallel RGB, I2C capacitive touch. |
| Graphics acceleration | `DMA2D` | Enable | Use for framebuffer fills, blits, and pixel format conversion. |
| Framebuffer memory | `FMC` | `SDRAM 1`, `SDCKE0 + SDNE0`, 16-bit data | Choose the final SDRAM IC before locking row/column/timing values. |
| Chart/log storage | `SDMMC2` + FatFs | Enable later | Store chart/map tiles and logs on microSD. |
| Battery monitor | `ADC1` | `BATTERY_VSENSE` pin TBD | Use a resistor divider; do not feed pack voltage directly into the MCU. |
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

Use one shared I2C bus for the pressure and inertial sensors unless the display pin map forces a move:

| Device | Signal | STM32 Pin | Notes |
| --- | --- | --- | --- |
| ICM-20948 | SCL | `PB8 / I2C1_SCL` provisional | Conflicts with some LTDC/SDMMC2 alternate functions; confirm in CubeMX. |
| ICM-20948 | SDA | `PB9 / I2C1_SDA` provisional | Conflicts with some LTDC/SDMMC2 alternate functions; confirm in CubeMX. |
| BMP390 | SCL | `PB8 / I2C1_SCL` provisional | Same bus as IMU. |
| BMP390 | SDA | `PB9 / I2C1_SDA` provisional | Same bus as IMU. |
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
| TXD | `PA10 / USART1_RX` provisional | GPS to STM32. Confirm this does not conflict with LTDC/USB choices. |
| RXD | `PA9 / USART1_TX` provisional | STM32 to GPS for UBX config. Confirm this does not conflict with LTDC/USB choices. |
| TIMEPULSE | GPIO TBD | Optional PPS/timepulse input. |
| EXTINT | GPIO TBD | Optional wake/interrupt. |
| RESET_N | GPIO TBD | Optional reset control. |

Hardware notes:

- Include a u.FL connector and/or ceramic patch antenna footprint.
- Follow u-blox RF layout guidance for antenna trace, keepout, and ground.
- Add backup power if retaining GNSS time/config is desired.

## Bluetooth Co-Processor

Use an nRF52840 as a Bluetooth LE co-processor on the board. Initial host link is asynchronous UART because it is simple and maps cleanly into STM32 firmware. SPI can be reserved if higher throughput is needed later, but it is not needed for first bring-up.

| nRF52840 Signal | STM32 Pin | Notes |
| --- | --- | --- |
| nRF UART RX | `PD8 / USART3_TX` provisional | STM32 to nRF52840. |
| nRF UART TX | `PD9 / USART3_RX` provisional | nRF52840 to STM32. |
| nRF RTS | GPIO TBD | Optional flow control; reserve if routing allows. |
| nRF CTS | GPIO TBD | Optional flow control; reserve if routing allows. |
| nRF RESET_N | GPIO TBD | Strongly recommended. |
| nRF BOOT/DFU | GPIO TBD | Recommended for firmware update/recovery. |
| nRF IRQ | GPIO TBD | Optional host wake/attention line. |
| nRF SWDIO | nRF debug header | Program/debug nRF52840 separately. |
| nRF SWDCLK | nRF debug header | Program/debug nRF52840 separately. |

nRF52840 hardware notes:

- Add the Nordic-recommended 32 MHz crystal and matching/load capacitors if required by the chosen reference design.
- Add a 32.768 kHz crystal if low-power BLE timing needs it.
- Follow Nordic RF layout guidance closely: antenna keepout, matching network, short feed, ground stitching.
- Decide chip antenna, PCB antenna, or u.FL.
- Keep a dedicated nRF SWD header or tag-connect footprint.
- Consider UART hardware flow control if high-rate telemetry will be streamed.

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
| Battery voltage monitor | Resistor divider into `ADC1` pin TBD. |
| Current monitor | Optional shunt/current-sense amplifier. |
| Backlight supply | Separate LED boost/current driver for the Newhaven backlight. |
| 3.3 V rail | STM32, GNSS, nRF52840, BMP390, touch, SD, and display logic. |
| 1.8 V rail | ICM-20948 if kept at 1.8 V. |

## SD Card

Use `SDMMC2` because that is the SDMMC instance CubeMX exposed cleanly for the STM32H723ZGT6 setup.

| SD Signal | STM32 Pin | Notes |
| --- | --- | --- |
| D0 | CubeMX-selected `SDMMC2_D0` | 4-bit SDMMC. |
| D1 | CubeMX-selected `SDMMC2_D1` | 4-bit SDMMC. |
| D2 | CubeMX-selected `SDMMC2_D2` | 4-bit SDMMC. |
| D3 | CubeMX-selected `SDMMC2_D3` | 4-bit SDMMC. |
| CK | CubeMX-selected `SDMMC2_CK` | Controlled impedance/routing care. |
| CMD | CubeMX-selected `SDMMC2_CMD` | Pull-up per SD requirements. |
| CD | GPIO TBD | Optional card detect. |

Do not use the auto-direction voltage converter mode unless the schematic includes the matching external SD voltage translator.

## Display

Current display direction: Newhaven `NHD-5.0-800480TF-ATXL-CTP`.

| Display Signal | STM32 / Board Connection | Notes |
| --- | --- | --- |
| `R0-R7` | `LTDC_R0-R7` or reduced color subset | Full RGB888 consumes many pins; use CubeMX to resolve conflicts. |
| `G0-G7` | `LTDC_G0-G7` or reduced color subset | Full RGB888 consumes many pins; use CubeMX to resolve conflicts. |
| `B0-B7` | `LTDC_B0-B7` or reduced color subset | Full RGB888 consumes many pins; use CubeMX to resolve conflicts. |
| `CLKIN` | `LTDC_CLK` | Pixel clock. |
| `HSD` | `LTDC_HSYNC` | Horizontal sync. |
| `VSD` | `LTDC_VSYNC` | Vertical sync. |
| `DEN` | `LTDC_DE` | Display enable. |
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
| Data | 16-bit, `D0-D15`. |
| Byte masks | Enable `NBL0/NBL1` if the SDRAM has `LDQM/UDQM`. |
| Address | Match the selected SDRAM IC row/column geometry. |
| Bank address | Match selected SDRAM IC, usually `BA0/BA1` for 4 internal banks. |
| Clock/chip enable | `SDCLK`, `SDCKE0`, `SDNE0`. |
| Control | `SDNRAS`, `SDNCAS`, `SDNWE`. |
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

Add separate debug access for the nRF52840.

## Open Pin Decisions

- Final LTDC color depth and exact RGB pin mapping.
- Exact SDRAM part number and timing values.
- SDMMC2 pin route that does not collide with LTDC/FMC.
- Whether I2C1 stays on `PB8/PB9` or moves to another I2C bus because of display conflicts.
- nRF52840 GPIO choices for reset, DFU, IRQ, RTS, and CTS.
- GPS TIMEPULSE and reset pins.
- IMU and BMP390 interrupt pins.
- SD card detect pin.
- USB VBUS sense option.
- Battery sense ADC pin.
- Power tree and voltage domains for sensor I2C.
