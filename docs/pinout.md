# moVe Custom Board Pinout Draft

Target MCU: `STM32H745ZI`

Board: custom KiCad single-board design

This is the first-pass pin/peripheral plan. Confirm every assignment in STM32CubeMX against the exact package, display choice, and PCB routing before locking the schematic.

## Peripheral Summary

| Function | STM32 Peripheral | Proposed Pins | Notes |
| --- | --- | --- | --- |
| ICM-20948 + BMP390 | `I2C1` | `PB8` SCL, `PB9` SDA | Use correct pull-up voltage for the bus domain. |
| MAX-M10S GNSS | `USART1` | `PA9` TX, `PA10` RX | STM32 TX to GPS RX, GPS TX to STM32 RX. |
| nRF52840 Bluetooth | `USART3` | `PD8` TX, `PD9` RX | Primary host link between STM32 and nRF52840. |
| nRF52840 control | GPIO | TBD | Add `nRF_RESET`, `nRF_BOOT/DFU`, and optional IRQ line. |
| SD card | `SDMMC1` | `PC8` D0, `PC9` D1, `PC10` D2, `PC11` D3, `PC12` CK, `PD2` CMD | Add card-detect GPIO if the socket supports it. |
| SPI display | `SPI1` | `PA5` SCK, `PA6` MISO, `PA7` MOSI | Add display `CS`, `DC`, `RST`, and `BL` GPIOs. |
| USB-C device | `USB_OTG_FS` or `USB_OTG_HS` | FS: `PA11` DM, `PA12` DP | Use HS only if an external ULPI PHY is included. |
| Debug | `SWD` | `PA13` SWDIO, `PA14` SWCLK | Add `NRST`, 3V3, and GND to debug header. |

## Sensor Bus

Use one shared I2C bus for the pressure and inertial sensors:

| Device | Signal | STM32 Pin | Notes |
| --- | --- | --- | --- |
| ICM-20948 | SCL | `PB8 / I2C1_SCL` | Check voltage domain. |
| ICM-20948 | SDA | `PB9 / I2C1_SDA` | Check voltage domain. |
| BMP390 | SCL | `PB8 / I2C1_SCL` | Same bus as IMU. |
| BMP390 | SDA | `PB9 / I2C1_SDA` | Same bus as IMU. |
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
| TXD | `PA10 / USART1_RX` | GPS to STM32. |
| RXD | `PA9 / USART1_TX` | STM32 to GPS for UBX config. |
| TIMEPULSE | GPIO TBD | Optional PPS/timepulse input. |
| EXTINT | GPIO TBD | Optional wake/interrupt. |
| RESET_N | GPIO TBD | Optional reset control. |

Hardware notes:

- Include a u.FL connector and/or ceramic patch antenna footprint.
- Follow u-blox RF layout guidance for antenna trace, keepout, and ground.
- Add backup power if retaining GNSS time/config is desired.

## Bluetooth Co-Processor

Use an nRF52840 as a Bluetooth LE co-processor on the board. Initial host link is UART because it is simple and maps cleanly into STM32 firmware. SPI can be reserved if higher throughput is needed later.

| nRF52840 Signal | STM32 Pin | Notes |
| --- | --- | --- |
| nRF UART RX | `PD8 / USART3_TX` | STM32 to nRF52840. |
| nRF UART TX | `PD9 / USART3_RX` | nRF52840 to STM32. |
| nRF RTS | GPIO TBD | Optional flow control. |
| nRF CTS | GPIO TBD | Optional flow control. |
| nRF RESET | GPIO TBD | Strongly recommended. |
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

For a simple USB device port, use USB full-speed first:

| USB-C Signal | STM32 Pin | Notes |
| --- | --- | --- |
| D- | `PA11 / USB_OTG_FS_DM` | Route as differential pair. |
| D+ | `PA12 / USB_OTG_FS_DP` | Route as differential pair. |
| CC1/CC2 | CC resistors/controller | Needed for USB-C attach behavior. |
| VBUS sense | GPIO/USB VBUS pin TBD | Depends on power architecture. |

If true USB HS is required, include an external ULPI PHY and use CubeMX `USB_OTG_HS` with the ULPI pin set. USB-C connector alone does not make the interface high-speed.

## SD Card

| SD Signal | STM32 Pin | Notes |
| --- | --- | --- |
| D0 | `PC8 / SDMMC1_D0` | 4-bit SDMMC. |
| D1 | `PC9 / SDMMC1_D1` | 4-bit SDMMC. |
| D2 | `PC10 / SDMMC1_D2` | 4-bit SDMMC. |
| D3 | `PC11 / SDMMC1_D3` | 4-bit SDMMC. |
| CK | `PC12 / SDMMC1_CK` | Controlled impedance/routing care. |
| CMD | `PD2 / SDMMC1_CMD` | Pull-up per SD requirements. |
| CD | GPIO TBD | Optional card detect. |

## Display

Display hardware is not locked yet. For an SPI TFT starting point:

| Display Signal | STM32 Pin | Notes |
| --- | --- | --- |
| SCK | `PA5 / SPI1_SCK` | SPI display clock. |
| MISO | `PA6 / SPI1_MISO` | Optional, depending display. |
| MOSI | `PA7 / SPI1_MOSI` | Display data. |
| CS | GPIO TBD | Chip select. |
| DC | GPIO TBD | Data/command select. |
| RST | GPIO TBD | Display reset. |
| BL | PWM GPIO TBD | Backlight control. |

If the final display is RGB/LTDC or parallel, this section should be replaced before schematic lock.

## Debug And Boot

| Function | STM32 Pin | Notes |
| --- | --- | --- |
| SWDIO | `PA13` | STM32 debug header. |
| SWCLK | `PA14` | STM32 debug header. |
| NRST | `NRST` | STM32 reset. |
| BOOT0 | `BOOT0` | Add pulldown and optional boot strap/test point. |

Add separate debug access for the nRF52840.

## Open Pin Decisions

- Exact display type and interface.
- nRF52840 GPIO choices for reset, DFU, IRQ, RTS, and CTS.
- GPS TIMEPULSE and reset pins.
- IMU and BMP390 interrupt pins.
- SD card detect pin.
- USB FS vs USB HS with external ULPI PHY.
- Power tree and voltage domains for sensor I2C.
