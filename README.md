# moVe

High-performance (ish) STM32H7 sensor suite and navigation platform for autonomous drones, planes, cars, and boats. Featuring ICM-20948, BMP390, u-blox M10 GNSS, nRF52840 Bluetooth LE, and an RGB TFT cockpit display path. Will include non-volatile storage for datalogging, map/chart assets, and USB.

To include a header for possible 3.3V Lithium Ion Cells

PCA9306 Level Shifter(s)

This is a plain C STM32 HAL implementation for:

## Custom Board

- Main MCU: STM32H723ZGT6.
- STM32H723ZG datasheet: [STMicroelectronics PDF](https://www.st.com/resource/en/datasheet/stm32h723zg.pdf).
- Designed as a custom single board in KiCad.
- Initial pinout draft: `docs/pinout.md`.
- nRF52840 Bluetooth LE co-processor on a UART host link, with reset/DFU/debug lines reserved.
- Display direction: Newhaven `NHD-5.0-800480TF-ATXL-CTP`, driven with LTDC RGB, DMA2D, and external SDRAM.
- Storage direction: `SDMMC2` microSD in 4-bit mode for logs and chart/map tiles.

## ICM-20948 + Madgwick AHRS for STM32H7
[Datasheet](https://d17t6iyxenbwp1.cloudfront.net/s3fs-public/2026-01/DS-000189-ICM-20948-v1.6.pdf?VersionId=XteIUeEiGGKWJDQjSzr3D2K3OIitFvHY)

- To run on 1.8V Bus - Low-Dropout Regulator (MIC5225)
- ICM-20948 by InvenSense TDK
- ICM-20948 accelerometer and gyroscope over I2C. 
- ICM-20948 contains the AK09916 magnetometer (allegedly according to adafruit) through ICM-20948 bypass mode.
- Madgwick 6-axis or 9-axis attitude filtering.
- Roll, pitch, yaw, and aircraft/drone display ready instrument values.
- Roll/Pitch applications for marine use

## BMP390 + Simple Kalman Filtering for STM32H7
[Datasheet](https://www.mouser.com/datasheet/3/1046/1/bst_bmp390_ds002.pdf)
- To run on 3.3V Bus
- BMP390 by Bosch 
- I2C, SPI capabilities
- Also, temperature capabilities 
- Compensated pressure and temperature over I2C.
- Pressure altitude conversion with configurable sea-level pressure.
- Two-state Kalman filter for altitude and vertical speed.

## U-Blox MAX-M10S for STM32H7

[Datasheet](https://content.u-blox.com/sites/default/files/MAX-M10S_DataSheet_UBX-20035208.pdf)
- To run on 3.3V Bus
- moving map/navigation
- Outputs MSL elevation data, Ground Speed, Lat/Long Coordinates
- Will include uFL connector, as well as ceramic patch antenna by default
- Parses both UBX binary navigation messages and NMEA sentences.
- GPS ground speed feeds the airspeed display as a fallback speed source.

## nRF52840 Bluetooth LE Co-Processor

- To run on 3.3V bus.
- Host interface starts as asynchronous UART from STM32H723ZGT6 to nRF52840.
- Optional RTS/CTS hardware flow control should be reserved in the schematic.
- Include nRF reset, DFU/boot, IRQ, and separate SWD debug access.
- RF layout should follow Nordic guidance with antenna keepout and matching network.

## Graphical Display Path

- Newhaven `NHD-5.0-800480TF-ATXL-CTP` 5 inch, 800x480 RGB TFT with capacitive touch.
- STM32 `LTDC` drives the parallel RGB display interface.
- STM32 `DMA2D` accelerates fills, blits, and pixel format conversion.
- External SDRAM on `FMC SDRAM 1` holds the framebuffer; start with a 16-bit SDRAM bus.
- Use `RGB565` for the first framebuffer format even though the physical panel interface is RGB888.
- Touch uses I2C plus touch interrupt/reset GPIOs.
- Backlight needs a separate LED boost/current driver, not a direct STM32 GPIO.
- FAA sectionals/charts should be preprocessed into raster tiles and loaded from SD instead of rendering full PDFs on the MCU.

## Parts List

- STM32H723ZGT6 MCU
- nRF52840 Bluetooth LE SoC or module
- U-Blox MAX-M10S GNSS receiver
- ICM-20948 9-axis IMU
- BMP390 pressure/temperature sensor
- Newhaven `NHD-5.0-800480TF-ATXL-CTP` RGB TFT with capacitive touch
- 16-bit external SDRAM for framebuffer storage
- Backlight LED boost/current driver for the TFT
- PCA9306 level shifter(s), if mixed-voltage I2C is kept
- MIC5225 or equivalent low-noise LDO for 1.8 V rail
- 3.3 V regulator sized for STM32, GNSS, nRF52840, SD, and display
- USB-C Connector (5V 500mA)
- SD card socket
- Optional battery connector and protected 2-cell input path
- ADC battery-sense divider and optional current-sense amplifier
- GNSS antenna path: u.FL and/or ceramic patch antenna
- nRF52840 antenna path: chip antenna, PCB antenna, or u.FL with matching network
- STM32 SWD header
- nRF52840 SWD header
- Reset and boot/DFU buttons or test pads

## Files (so far) will update

- `Core/Inc/icm20948.h`
- `Core/Src/icm20948.c`
- `Core/Inc/madgwick.h`
- `Core/Src/madgwick.c`
- `Core/Inc/icm20948_madgwick_example.h`
- `Core/Src/icm20948_madgwick_example.c`
- `Core/Inc/navigation_fusion.h`
- `Core/Src/navigation_fusion.c`
- `Core/Src/main.c`
- `moVe.ioc`
- `docs/pinout.md`
- `Core/Inc/bmp390.h`
- `Core/Src/bmp390.c`
- `Core/Inc/kalman_altitude.h`
- `Core/Src/kalman_altitude.c`
- `Core/Inc/max_m10s.h`
- `Core/Src/max_m10s.c`
- `Aircraft/Inc/aircraft_instruments.h`
- `Aircraft/Src/aircraft_instruments.c`
- `Aircraft/Inc/attitude_indicator.h`
- `Aircraft/Src/attitude_indicator.c`
- `Aircraft/Inc/turn_slip.h`
- `Aircraft/Src/turn_slip.c`
- `Aircraft/Inc/heading_indicator.h`
- `Aircraft/Src/heading_indicator.c`
- `Aircraft/Inc/altimeter.h`
- `Aircraft/Src/altimeter.c`
- `Aircraft/Inc/airspeed_indicator.h`
- `Aircraft/Src/airspeed_indicator.c`
- `Aircraft/Inc/vertical_speed_indicator.h`
- `Aircraft/Src/vertical_speed_indicator.c`
- `Aircraft/Inc/aircraft_math.h`
- `Aircraft/Src/aircraft_math.c`

## STM32CubeIDE Use

Copy the `Core/Inc`, `Core/Src`, `Aircraft/Inc`, and `Aircraft/Src` files into your STM32H7 project. Enable I2C in CubeMX, then make sure the example uses the matching handle:

```c
extern I2C_HandleTypeDef hi2c1;
```

The Cube-style entry point is `Core/Src/main.c`. It shows the expected HAL init order for the H723 board direction, including I2C sensors, GPS UART, nRF UART, ADC battery sense, CRC, FMC SDRAM, DMA2D, LTDC, SDMMC2, USB OTG HS device mode with internal FS PHY, GPS UART byte reception, and the navigation fusion update loop.

Legacy compatibility wrappers are still available. Call this once after `MX_I2C1_Init()`:

```c
Attitude_Init();
```

Call this repeatedly from your main loop or a fixed-rate task:

```c
Attitude_Update();
```

The latest Euler angles are in:

```c
#include "icm20948_madgwick_example.h"

attitude_deg.roll_deg;
attitude_deg.pitch_deg;
attitude_deg.yaw_deg;
```

The display-ready aircraft instrument values are in:

```c
#include "icm20948_madgwick_example.h"

aircraft_display->attitude.roll_deg;
aircraft_display->attitude.pitch_deg;
aircraft_display->turn_slip.turn_rate_deg_s;
aircraft_display->turn_slip.standard_rate_fraction;
aircraft_display->turn_slip.slip_ball;
aircraft_display->heading.display_heading_deg;
aircraft_display->altimeter.display_altitude_ft;
aircraft_display->vsi.vertical_speed_fpm;
aircraft_display->airspeed.display_true_airspeed_kt;
```

The aircraft display layer is split by instrument. `Aircraft/Src/aircraft_instruments.c` coordinates the attitude indicator, turn-and-slip, heading card, altimeter, VSI, and airspeed modules without owning their display math.

## Notes

- Gyro values are converted from degrees/second to radians/second before entering Madgwick.
- The driver configures accel at +/-16 g and gyro at +/-2000 dps.
- The magnetometer is read at 100 Hz. If no new magnetometer data is ready, the filter falls back to the IMU-only Madgwick update for that frame.
- Real yaw quality depends heavily on magnetometer hard-iron and soft-iron calibration. Use `ICM20948_SetMagCalibration()` after you measure your board's calibration values.
- If yaw moves the wrong way, align the AK09916 axes to the accel/gyro frame with `ICM20948_SetMagAxisTransform()`.
- Set local magnetic declination with `AircraftInstruments_SetDeclination()` if the heading display should show true heading instead of magnetic heading.
- Set local altimeter pressure with `BMP390_SetSeaLevelPressure()` before converting pressure to altitude.
- The airspeed indicator displays estimated true airspeed when it has GPS speed plus BMP390 static pressure and temperature. If temperature or pressure are unavailable, it falls back to pressure-altitude ISA correction, then GPS ground speed.
- Aircraft use needs independent validation, redundancy, failure annunciation, and compliance work before it can be trusted as flight-critical instrumentation.
- If AD0 is high on your board, initialize with `ICM20948_ADDR_AD0_HIGH` instead of `ICM20948_ADDR_AD0_LOW`.
- Link with the math library if your toolchain requires it, usually by adding `-lm`.
