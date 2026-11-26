# moVe test platform — KiCad hardware (rev A)

A round **4.2 in (106.68 mm)**, **4-layer** bring-up board for the moVe STM32H723ZGT6 platform. It carries every
subsystem from the main README: SDRAM framebuffer, RGB TFT + capacitive touch, microSD, ICM-20948, BMP390,
MAX-M10S GNSS, NORA-B261 Bluetooth LE, the aircraft MQ-7 CO sensor, USB-C and a 2S battery input.

* Schematic: [`moVe_test_schematic.pdf`](moVe_test_schematic.pdf) (8 pages) · KiCad project: [`moVe_test.kicad_pro`](moVe_test.kicad_pro)
* Status: **fully routed — 521/521 connections, DRC 0 errors, 0 unconnected**, 146 parts
* Format: KiCad 7 (KiCad 8/9 open it and offer to upgrade)

<p align="center">
  <img src="images/board_top.png" width="48%" alt="Top side: copper, silkscreen and outline">
  <img src="images/board_bottom.png" width="48%" alt="Bottom side (mirrored): copper and silkscreen">
</p>
<p align="center"><em>Top side (left) and bottom side, mirrored (right).</em></p>

---

## Layer stackup — 4 layers

| Layer | Use |
|---|---|
| L1 `F.Cu` | Signals + components, GND pour |
| L2 `In1.Cu` | Solid GND plane |
| L3 `In2.Cu` | Solid +3V3 plane |
| L4 `B.Cu` | Signals + MCU/SDRAM decoupling, GND pour |

1.6 mm board (e.g. JLC04161H-7628). Rules: 0.15 mm track / 0.127 mm clearance, 0.6/0.3 mm vias, power 0.5 mm,
RF 0.36 mm (≈50 Ω microstrip over L2 with 7628 prepreg — confirm with your fab's impedance calculator).

**Why 4 layers:** 2 layers can't give the ~100 MHz SDRAM bus, the 24-bit LTDC bus, USB and the two 50 Ω RF feeds an
unbroken reference plane. The routed result confirms 4 is enough: every signal fits on L1/L4 with L2/L3 left as
solid planes (521 connections, 498 vias, ~7.1 m of copper). 6 layers is the upgrade path only if you want the SDRAM
bus length-matched on an inner stripline layer.

<table>
  <tr>
    <td><img src="images/layer_1_F.Cu_top_copper.png" alt="Layer 1 top copper"></td>
    <td><img src="images/layer_2_In1.Cu_GND_plane.png" alt="Layer 2 GND plane"></td>
  </tr>
  <tr>
    <td><img src="images/layer_3_In2.Cu_3V3_plane.png" alt="Layer 3 3V3 plane"></td>
    <td><img src="images/layer_4_B.Cu_bottom_copper.png" alt="Layer 4 bottom copper"></td>
  </tr>
  <tr>
    <td><img src="images/layer_5_F.Silkscreen_top.png" alt="Top silkscreen and mask"></td>
    <td><img src="images/layer_6_B.Silkscreen_bottom.png" alt="Bottom silkscreen and mask"></td>
  </tr>
</table>

Both copper layers together (red = top, blue = bottom):

<p align="center"><img src="images/board_copper.png" width="70%" alt="Top and bottom copper overlaid"></p>

---

## Schematic

Each sheet is organized into titled functional blocks: supply arrows point up and GND down, same-net power
pins are bussed, net names sit on wires for sheet-local nets, and global flag labels mark nets shared between
sheets. Text placement is collision-checked, so no text overlaps.

| Sheet | Blocks |
|---|---|
| [Power](images/sch_2_power.png) | USB-C input, power path (USB / 2S battery), 3.3 V and 1.8 V regulators, voltage sense, power LED, test points, mounting holes |
| [MCU](images/sch_3_mcu.png) | STM32H723ZGT6, supply decoupling, 25 MHz / 32.768 kHz clocks, reset and boot, SWD, status LEDs, expansion header |
| [Memory](images/sch_4_memory.png) | SDRAM framebuffer, microSD (SDMMC2) |
| [Display](images/sch_5_display.png) | 40-pin RGB TFT connector, capacitive touch, TPS61165 backlight driver |
| [Sensors](images/sch_6_sensors.png) | ICM-20948 (1.8 V), BMP390, PCA9306 I2C level shift, IMU INT level shift |
| [Radio](images/sch_7_radio.png) | MAX-M10S GNSS, NORA-B261 Bluetooth LE |
| [CO sensor](images/sch_8_co_sensor.png) | MQ-7 heater and sense (aircraft build) |

<details open><summary><b>Power</b></summary><img src="images/sch_2_power.png" alt="Power sheet"></details>
<details><summary><b>MCU</b></summary><img src="images/sch_3_mcu.png" alt="MCU sheet"></details>
<details><summary><b>Memory</b></summary><img src="images/sch_4_memory.png" alt="Memory sheet"></details>
<details><summary><b>Display</b></summary><img src="images/sch_5_display.png" alt="Display sheet"></details>
<details><summary><b>Sensors</b></summary><img src="images/sch_6_sensors.png" alt="Sensors sheet"></details>
<details><summary><b>Radio</b></summary><img src="images/sch_7_radio.png" alt="Radio sheet"></details>
<details><summary><b>CO sensor</b></summary><img src="images/sch_8_co_sensor.png" alt="CO sensor sheet"></details>

For full resolution open the [PDF](moVe_test_schematic.pdf) or the project in KiCad.

---

## Pin assignments added for this board

These were TBD in [`docs/pinout.md`](../../docs/pinout.md) (now updated). Mirror them in `moVe.ioc` (CubeMX) before
generating firmware.

| Signal | STM32 pin | Notes |
|---|---|---|
| BLE_CTS / BLE_RTS | PD11 / PD12 | USART3 CTS/RTS (AF7). MCU RTS → NORA CTS (F9), NORA RTS (F8) → MCU CTS |
| BLE_RESET_N, BLE_SW1, BLE_SW2 | PD13, PD4, PD5 | NORA J3, D8, C8 |
| BLE_WAKE | PC12 | NORA J9; shared with expansion header pin 7 |
| GPS_RESET_N, GPS_EXTINT, GPS_TIMEPULSE | PG3, PG14, PE2 | TIMEPULSE on EXTI2 |
| IMU_INT, BARO_INT, TOUCH_INT_N, SD_DETECT | PE3, PB4, PC13, PC8 | separate EXTI lines 3 / 4 / 13 / 8 |
| TOUCH_RST_N, LCD_STBYB | PB12, PB13 | STBYB has a 10k pull-down (panel in standby until firmware enables it) |
| BL_PWM, MQ7_HEAT | PB8 (TIM16_CH1), PB9 (TIM17_CH1) | independent timers |
| MQ7_ADC, VBAT_SENSE, VBUS_SENSE | PC5 (ADC12_INP8), PC4, PA9 | VBUS 10k/20k; VBAT 100k/33k (8.4 V → 2.08 V) |
| LED1, LED2 | PF6, PF7 | |
| SWO | PB3 | 10-pin Cortex SWD header |
| Expansion header J2 | — | 3V3, GND, SCL, SDA, PF8, PF9, PC12/BLE_WAKE, VSYS |

PD7 and PB2 are left unused.

## Design notes worth checking

* **Power:** USB-C VBUS → 1.1 A PTC → `+5V` (MQ-7 heater) → SS34 → `VSYS`; 2S battery (JST-PH) → SS34 → `VSYS`.
  VSYS → TLV1117-33 → +3V3 → AP2112K-1.8 → +1V8. At 8.4 V battery input the TLV1117 dissipates ~1.5 W; consider a buck converter for battery use.
* **MCU:** 25 MHz HSE crystal placed beside PH0/PH1 (both lines < 8 mm, no vias), 32.768 kHz LSE, VCAP pins tied with
  2 × 2.2 µF, VDDA/VREF+ behind a ferrite. Firmware must use `PWR_LDO_SUPPLY` (the H723ZG has no SMPS).
* **ICM-20948:** VDD and VDDIO on 1.8 V behind a PCA9306; INT level-shifted to 3.3 V with a BSS138.
  **AD0 is tied high → I2C address 0x69** — initialise with `ICM20948_ADDR_AD0_HIGH`.
* **SDRAM:** the symbol is MT48LC16M16A2P so FMC_A12 (PG2) reaches pin 36. The AS4C4M16SA is fitted (pin 36 = NC);
  the Micron 32 MB part drops in.
* **NORA-B261:** footprint follows data-sheet Fig. 5. Fig. 2 also shows GND pads K3/K7 that Fig. 5 omits; they're
  left out — verify. ANT → 0R + two DNP 0402 shunts (pi match) → u.FL.
* **MAX-M10S:** u.FL only (passive antenna, no VCC_RF bias); VIO_SEL open; SAFEBOOT_N on a test pad.
* **MQ-7:** AO3400A low-side heater switch, 10k load resistor, 10k/20k divider into the ADC.
* **Display connectors:** Hirose FH12-40S-0.5SH footprint for the 40-pin FPC (Newhaven lists Molex 54104-4031 —
  check the contact side before ordering) and Molex 200528-0060 for the 6-pin touch FFC. Backlight: TPS61165 with
  6.8 Ω sense ≈ 29 mA.

## Verification

* Schematic netlist ↔ `generator/design.py`: 0 mismatches. PCB pad nets ↔ schematic: 0 mismatches.
* DRC: 0 errors, 0 unconnected. The 20 remaining `lib_footprint_mismatch` warnings come from moving 50 overlapping
  silkscreen outline segments to the Fab layer; **don't run "Update footprints from library"** on those parts or the
  overlaps come back.
* Silkscreen: 0 overlaps, 0 silk-over-pad; all references visible.
* ERC has not been run (KiCad 7's command line has no ERC) — run Inspect → ERC in KiCad.

**Before fab:** routes are autorouted, not length-matched — FMC nets run 15–88 mm (SDCLK 45 mm), fine for bring-up at
≤ 100 MHz. USB D+/D− are routed separately (OK for 12 Mb/s full-speed). VBUS at the USB-C pins uses 0.3 mm traces
(the USB4105 pads are too small for 0.5 mm); the run is short and fine for 1 A.

## Regenerating

Everything is generated from [`generator/design.py`](generator/design.py) (one netlist definition drives the
schematic and the board) with the KiCad 7 Python API (`pcbnew`):

```sh
cd hardware/moVe_test/generator
python3 gen_sch.py              # schematic sheets, project symbol/footprint libraries
kicad-cli sch export netlist -o ../moVe_test.net ../moVe_test.kicad_sch
python3 check_net.py ../moVe_test.net   # netlist must match design.py
```

The board was placed with `gen_pcb.py`, fanned out with `fanout.py`, autorouted with
[Freerouting](https://github.com/freerouting/freerouting) (`prep_route.py`, `import_ses.py`), then finished with
`ripup_route.py` and `silk_fix.py`. Set `KICAD7_SYMBOL_DIR`, `KICAD7_FOOTPRINT_DIR` and `FREEROUTING_JAR` if your
KiCad libraries or the Freerouting jar aren't in the Linux default locations. Re-running placement discards the
current routing.
