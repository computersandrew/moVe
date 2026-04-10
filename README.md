# moVe

High-performance (ish) STM32H7 sensor suite and navigation platform for autonomous drones, planes, cars, and boats. Featuring ICM-20948, BMP390, and u-blox M10 GNSS. Will include an option for Non-Volatile Storage for Datalogging and USB. Will debate 802.11ac/Bluetooth 4.0LE implementation

This is a plain C STM32 HAL implementation for:

## ICM-20948 + Madgwick AHRS for STM32H7
[Datasheet](https://d17t6iyxenbwp1.cloudfront.net/s3fs-public/2026-01/DS-000189-ICM-20948-v1.6.pdf?VersionId=XteIUeEiGGKWJDQjSzr3D2K3OIitFvHY)

- To run on 3.3V Bus
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

## U-Blox MAX-M10S for STM32H7

[Datasheet](https://content.u-blox.com/sites/default/files/MAX-M10S_DataSheet_UBX-20035208.pdf)
- To run on 3.3V Bus
- moving map/navigation
- Outputs MSL elevation data, Ground Speed, Lat/Long Coordinates
- Will include uFL connector, as well as ceramic patch antenna by default

## Files (so far) will update

- `Core/Inc/icm20948.h`
- `Core/Src/icm20948.c`
- `Core/Inc/madgwick.h`
- `Core/Src/madgwick.c`
- `Core/Inc/icm20948_madgwick_example.h`
- `Core/Src/icm20948_madgwick_example.c`
- `Aircraft/Inc/aircraft_instruments.h`
- `Aircraft/Src/aircraft_instruments.c`

## STM32CubeIDE Use

Copy the `Core/Inc`, `Core/Src`, `Aircraft/Inc`, and `Aircraft/Src` files into your STM32H7 project. Enable I2C in CubeMX, then make sure the example uses the matching handle:

```c
extern I2C_HandleTypeDef hi2c1;
```

Call this once after `MX_I2C1_Init()`:

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
```

`Aircraft/Src/aircraft_instruments.c` is intentionally separate from the IMU and filter code. It is the display adapter for an attitude indicator, turn-and-slip, and heading card.

## Notes

- Gyro values are converted from degrees/second to radians/second before entering Madgwick.
- The driver configures accel at +/-16 g and gyro at +/-2000 dps.
- The magnetometer is read at 100 Hz. If no new magnetometer data is ready, the filter falls back to the IMU-only Madgwick update for that frame.
- Real yaw quality depends heavily on magnetometer hard-iron and soft-iron calibration. Use `ICM20948_SetMagCalibration()` after you measure your board's calibration values.
- If yaw moves the wrong way, align the AK09916 axes to the accel/gyro frame with `ICM20948_SetMagAxisTransform()`.
- Set local magnetic declination with `AircraftInstruments_SetDeclination()` if the heading display should show true heading instead of magnetic heading.
- Aircraft use needs independent validation, redundancy, failure annunciation, and compliance work before it can be trusted as flight-critical instrumentation.
- If AD0 is high on your board, initialize with `ICM20948_ADDR_AD0_HIGH` instead of `ICM20948_ADDR_AD0_LOW`.
- Link with the math library if your toolchain requires it, usually by adding `-lm`.
