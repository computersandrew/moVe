# moVe STM32 Implementation Checklist

## Firmware Structure

- [x] Split aircraft display logic by instrument.
- [x] Add `navigation_fusion` module for IMU, barometer, GPS, AHRS, altitude, and aircraft-display outputs.
- [x] Keep legacy `Attitude_Init()` / `Attitude_Update()` wrappers for compatibility.
- [x] Add Cube-style `main.c` skeleton.
- [ ] Replace skeleton declarations with STM32CubeMX-generated files.
- [ ] Decide bare-metal scheduler vs FreeRTOS.
- [ ] Add module-level fault/status outputs.

## STM32CubeMX Bring-Up

- [ ] Create `.ioc` project for the target STM32H7.
- [ ] Configure system clock.
- [ ] Configure I2C for ICM-20948 and BMP390.
- [ ] Configure UART for MAX-M10S.
- [ ] Configure display interface.
- [ ] Configure debug UART or SWO logging.
- [ ] Confirm all GPIO alternate functions.

## ICM-20948 AHRS

- [x] ICM-20948 accel/gyro driver.
- [x] AK09916 magnetometer read through bypass mode.
- [x] Madgwick 6-axis/9-axis filter.
- [x] Roll, pitch, yaw output.
- [ ] Confirm physical sensor axis mapping on hardware.
- [ ] Add accelerometer calibration.
- [ ] Add persistent gyro bias storage.
- [ ] Add magnetometer hard-iron/soft-iron calibration.
- [ ] Add IMU data-ready interrupt support.

## BMP390 Altitude

- [x] BMP390 pressure/temperature driver.
- [x] Pressure altitude conversion.
- [x] Kalman altitude and vertical-speed filter.
- [x] Altimeter display output.
- [x] VSI display output.
- [ ] Add local altimeter setting input.
- [ ] Add pressure/temperature sanity checks.
- [ ] Add barometer timeout and stale-data flags.

## MAX-M10S GPS

- [x] NMEA parser for `RMC`, `GGA`, and `VTG`.
- [x] UBX parser for `NAV-PVT` and `NAV-VELNED`.
- [x] GPS ground-speed output.
- [x] GPS altitude output.
- [ ] Add MAX-M10S boot configuration messages.
- [ ] Add UBX ACK/NAK handling.
- [ ] Configure output protocol mix.
- [ ] Configure navigation update rate.
- [ ] Add UART DMA or idle-line receive path.
- [ ] Add GPS no-fix and stale-data flags.

## Aircraft Instruments

- [x] Attitude indicator output.
- [x] Turn-and-slip output.
- [x] Heading output.
- [x] Altimeter output.
- [x] VSI output.
- [x] True-airspeed estimate with GPS fallback.
- [ ] Add instrument validity/annunciation state per display.
- [ ] Add unit selection where needed.
- [ ] Add display scaling constants.
- [ ] Add smoothing/tuning config storage.

## Airspeed

- [x] Use GPS ground speed as fallback.
- [x] Estimate true airspeed from GPS speed plus BMP390 pressure and temperature.
- [x] Fall back to pressure-altitude ISA correction if measured density is unavailable.
- [ ] Add pitot/static differential pressure sensor support.
- [ ] Add wind-aware correction if wind estimate becomes available.
- [ ] Clearly label source on display: TAS estimate vs GPS ground speed.

## Display

- [ ] Select display hardware and interface.
- [ ] Implement attitude indicator rendering.
- [ ] Implement turn/slip rendering.
- [ ] Implement heading card rendering.
- [ ] Implement altimeter rendering.
- [ ] Implement VSI rendering.
- [ ] Implement TAS/GS airspeed rendering.
- [ ] Add invalid-data annunciations.
- [ ] Add display refresh scheduler.

## Storage And Configuration

- [ ] Decide non-volatile storage: internal flash, EEPROM, FRAM, or SD.
- [ ] Store magnetometer calibration.
- [ ] Store gyro/accelerometer calibration.
- [ ] Store altimeter setting.
- [ ] Store magnetic declination.
- [ ] Store display/user preferences.
- [ ] Add config versioning.

## Fault Handling

- [ ] Add I2C bus recovery.
- [ ] Add per-sensor timeout counters.
- [ ] Add degraded AHRS modes.
- [ ] Add GPS fix-quality gating.
- [ ] Add barometer plausibility gating.
- [ ] Add startup self-test summary.
- [ ] Add runtime fault flags for display.

## Desktop Tests

- [ ] Add NMEA parser test vectors.
- [ ] Add UBX parser test vectors.
- [ ] Add Madgwick static-orientation checks.
- [ ] Add BMP390 compensation sample check.
- [ ] Add Kalman altitude step-response check.
- [ ] Add aircraft instrument conversion tests.

## Hardware Validation

- [ ] Confirm I2C pull-ups and voltage domains.
- [ ] Confirm PCA9306 level shifter behavior.
- [ ] Confirm GPS antenna path.
- [ ] Confirm BMP390 placement and venting.
- [ ] Confirm IMU mechanical alignment.
- [ ] Log real sensor data to SD or serial.
- [ ] Compare attitude against known reference.
- [ ] Compare altitude against known pressure source.
- [ ] Compare GPS speed/position against known receiver/app.
