"""moVe test platform - circuit definition (single source of truth for schematic + PCB)."""
from collections import OrderedDict, defaultdict

COMPS = []
_counters = defaultdict(int)
_BLOCK = [None]
BLOCK_ORDER = []

def B(name):
    """start a functional block (Adafruit-style dashed region) for subsequent parts"""
    _BLOCK[0] = name
    if name not in BLOCK_ORDER:
        BLOCK_ORDER.append(name)

class Comp:
    def __init__(self, ref, lib_id, value, footprint, sheet, pins, mpn="", dnp=False, note=""):
        self.ref, self.lib_id, self.value, self.footprint = ref, lib_id, value, footprint
        self.sheet, self.pins, self.mpn, self.dnp, self.note = sheet, pins, mpn, dnp, note
        self.group = None
        self.block = _BLOCK[0]

def _ref(prefix):
    _counters[prefix] += 1
    return f"{prefix}{_counters[prefix]}"

def add(prefix, lib_id, value, footprint, sheet, pins, **kw):
    c = Comp(_ref(prefix), lib_id, value, footprint, sheet, pins, **kw)
    COMPS.append(c)
    return c

FP_R = "Resistor_SMD:R_0603_1608Metric"
FP_C = "Capacitor_SMD:C_0603_1608Metric"
FP_C0805 = "Capacitor_SMD:C_0805_2012Metric"
FP_C1206 = "Capacitor_SMD:C_1206_3216Metric"

def R(sheet, val, a, b, fp=FP_R, **kw):
    return add("R", "Device:R", val, fp, sheet, {"1": a, "2": b}, **kw)

def C(sheet, val, a, b="GND", fp=FP_C, **kw):
    return add("C", "Device:C", val, fp, sheet, {"1": a, "2": b}, **kw)

def TP(sheet, net):
    return add("TP", "Connector:TestPoint", net.lstrip("+"), "TestPoint:TestPoint_Pad_D1.5mm", sheet, {"1": net})

# =============================================================== MCU
MCU = OrderedDict()
def _m(pin, net):
    MCU[pin] = net

for p, n in {
    "PA0": "SDMMC2_CMD", "PA1": "LTDC_R2", "PA2": "LTDC_R1", "PA3": "LTDC_B2", "PA4": "LTDC_VSYNC",
    "PA5": "LTDC_R4", "PA6": "LTDC_G2", "PA7": "FMC_SDNWE", "PA8": "LTDC_B3", "PA9": "VBUS_SENSE",
    "PA10": "LTDC_B1", "PA11": "USB_DM", "PA12": "USB_DP", "PA13": "SWDIO", "PA14": "SWCLK", "PA15": "LTDC_B6",
    "PB0": "LTDC_R3", "PB1": "LTDC_R6", "PB3": "SWO", "PB4": "BARO_INT", "PB5": "LTDC_B5",
    "PB6": "I2C1_SCL", "PB7": "I2C1_SDA", "PB8": "BL_PWM", "PB9": "MQ7_HEAT", "PB10": "LTDC_G4",
    "PB11": "BLE_UART_RX", "PB12": "TOUCH_RST_N", "PB13": "LCD_STBYB", "PB14": "GPS_UART_TX", "PB15": "GPS_UART_RX",
    "PC0": "LTDC_R5", "PC1": "LTDC_G5", "PC2_C": "FMC_SDNE0", "PC3_C": "FMC_SDCKE0", "PC4": "VBAT_SENSE",
    "PC5": "MQ7_ADC", "PC6": "LTDC_HSYNC", "PC7": "LTDC_G6", "PC8": "SD_DETECT", "PC9": "LTDC_G3",
    "PC10": "BLE_UART_TX", "PC11": "LTDC_B4", "PC12": "BLE_WAKE", "PC13": "TOUCH_INT_N",
    "PC14": "OSC32_IN", "PC15": "OSC32_OUT",
    "PD0": "FMC_D2", "PD1": "FMC_D3", "PD2": "LTDC_B7", "PD3": "LTDC_G7", "PD4": "BLE_SW1", "PD5": "BLE_SW2",
    "PD6": "SDMMC2_CK", "PD8": "FMC_D13", "PD9": "FMC_D14", "PD10": "FMC_D15",
    "PD11": "BLE_CTS", "PD12": "BLE_RTS", "PD13": "BLE_RESET_N", "PD14": "FMC_D0", "PD15": "FMC_D1",
    "PE0": "FMC_NBL0", "PE1": "FMC_NBL1", "PE2": "GPS_TIMEPULSE", "PE3": "IMU_INT", "PE4": "LTDC_B0",
    "PE5": "LTDC_G0", "PE6": "LTDC_G1",
    "PF0": "FMC_A0", "PF1": "FMC_A1", "PF2": "FMC_A2", "PF3": "FMC_A3", "PF4": "FMC_A4", "PF5": "FMC_A5",
    "PF6": "LED1", "PF7": "LED2", "PF8": "EXP_PF8", "PF9": "EXP_PF9", "PF10": "LTDC_DE", "PF11": "FMC_SDNRAS",
    "PF12": "FMC_A6", "PF13": "FMC_A7", "PF14": "FMC_A8", "PF15": "FMC_A9",
    "PG0": "FMC_A10", "PG1": "FMC_A11", "PG2": "FMC_A12", "PG3": "GPS_RESET_N", "PG4": "FMC_BA0", "PG5": "FMC_BA1",
    "PG6": "LTDC_R7", "PG7": "LTDC_CLK", "PG8": "FMC_SDCLK", "PG9": "SDMMC2_D0", "PG10": "SDMMC2_D1",
    "PG11": "SDMMC2_D2", "PG12": "SDMMC2_D3", "PG13": "LTDC_R0", "PG14": "GPS_EXTINT", "PG15": "FMC_SDNCAS",
    "PH0": "HSE_IN", "PH1": "HSE_OUT", "PDR_ON": "+3V3", "NRST": "NRST", "BOOT0": "BOOT0",
    "VBAT": "+3V3", "VDD": "+3V3", "VSS": "GND", "VSSA": "GND", "VDDA": "VDDA", "VREF+": "VDDA",
    "VDD33USB": "+3V3", "VCAP": "VCAP",
}.items():
    _m(p, n)
for i, p in enumerate(range(7, 16)):
    _m(f"PE{p}", f"FMC_D{4 + i}")

B("STM32H723ZGT6")
U_MCU = add("U", "MCU_ST_STM32H7:STM32H723ZGTx", "STM32H723ZGT6", "Package_QFP:LQFP-144_20x20mm_P0.5mm",
            "MCU", {"@byname": MCU}, mpn="STM32H723ZGT6")

S = "MCU"
B("Supply decoupling")
VDD_PINS = ["17", "30", "39", "52", "62", "72", "84", "108", "121", "131", "144"]
for _ in VDD_PINS:
    C(S, "100n", "+3V3").group = ("decap", U_MCU.ref)
C(S, "4.7u", "+3V3", fp=FP_C0805).group = ("decap", U_MCU.ref)
C(S, "4.7u", "+3V3", fp=FP_C0805).group = ("decap", U_MCU.ref)
C(S, "100n", "+3V3").group = ("decap", U_MCU.ref)  # VDD33USB
add("FB", "Device:FerriteBead_Small", "600R@100MHz", "Inductor_SMD:L_0603_1608Metric", S, {"1": "+3V3", "2": "VDDA"},
    mpn="BLM18PG601SN1D")
C(S, "1u", "VDDA").group = ("decap", U_MCU.ref)
C(S, "100n", "VDDA").group = ("decap", U_MCU.ref)
C(S, "100n", "VDDA").group = ("decap", U_MCU.ref)   # VREF+
C(S, "2.2u", "VCAP").group = ("decap", U_MCU.ref)
C(S, "2.2u", "VCAP").group = ("decap", U_MCU.ref)
B("Clocks - 25 MHz HSE / 32.768 kHz LSE")
# Clocks
add("Y", "Device:Crystal_GND24", "25MHz", "Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm", S,
    {"1": "HSE_IN", "2": "GND", "3": "HSE_OUT", "4": "GND"}, mpn="ABM8-25.000MHZ-B2-T (CL=10pF)")
C(S, "12p", "HSE_IN")
C(S, "12p", "HSE_OUT")
add("Y", "Device:Crystal", "32.768kHz", "Crystal:Crystal_SMD_3215-2Pin_3.2x1.5mm", S,
    {"1": "OSC32_IN", "2": "OSC32_OUT"}, mpn="ABS07-32.768KHZ-7-T (CL=7pF)")
C(S, "10p", "OSC32_IN")
C(S, "10p", "OSC32_OUT")
B("Reset and boot")
# Reset / boot
C(S, "100n", "NRST")
add("SW", "Switch:SW_Push", "RESET", "Button_Switch_SMD:SW_SPST_PTS645", S, {"1": "NRST", "2": "GND"},
    mpn="PTS645SK43SMTR92 LFS")
R(S, "10k", "BOOT0", "GND")
add("SW", "Switch:SW_Push", "BOOT0", "Button_Switch_SMD:SW_SPST_PTS645", S, {"1": "+3V3", "2": "BOOT0"},
    mpn="PTS645SK43SMTR92 LFS")
B("SWD debug")
# SWD
add("J", "Connector:Conn_ARM_JTAG_SWD_10", "SWD", "Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical_SMD", S,
    {"1": "+3V3", "2": "SWDIO", "3": "GND", "4": "SWCLK", "5": "GND", "6": "SWO", "7": None, "8": None,
     "9": "GND", "10": "NRST"}, mpn="FTSH-105-01-L-DV-K-P")
B("Status LEDs")
# Status LEDs
for led in ("LED1", "LED2"):
    R(S, "1k", led, led + "_K")
    add("D", "Device:LED", "LED green" if led == "LED1" else "LED blue", "LED_SMD:LED_0603_1608Metric", S,
        {"1": "GND", "2": led + "_K"})
B("Expansion header")
# Expansion header
add("J", "Connector_Generic:Conn_01x08", "EXPANSION", "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical", S,
    {"1": "+3V3", "2": "GND", "3": "I2C1_SCL", "4": "I2C1_SDA", "5": "EXP_PF8", "6": "EXP_PF9", "7": "BLE_WAKE",
     "8": "VSYS"})

# =============================================================== POWER
S = "Power"
B("USB-C input")
add("J", "Connector:USB_C_Receptacle_USB2.0_16P", "USB-C", "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal",
    S, {"A1": "GND", "A12": "GND", "B1": "GND", "B12": "GND", "A4": "VBUS", "A9": "VBUS", "B4": "VBUS", "B9": "VBUS",
        "A5": "USB_CC1", "B5": "USB_CC2", "A6": "USB_C_DP", "B6": "USB_C_DP", "A7": "USB_C_DM", "B7": "USB_C_DM",
        "A8": None, "B8": None, "S1": "GND"}, mpn="USB4105-GF-A-120")
R(S, "5.1k", "USB_CC1", "GND")
R(S, "5.1k", "USB_CC2", "GND")
add("U", "Power_Protection:USBLC6-2SC6", "USBLC6-2SC6", "Package_TO_SOT_SMD:SOT-23-6", S,
    {"1": "USB_C_DP", "6": "USB_DP", "3": "USB_C_DM", "4": "USB_DM", "2": "GND", "5": "VBUS"}, mpn="USBLC6-2SC6")
B("Power path - USB / 2S battery")
add("F", "Device:Polyfuse", "1.1A hold", "Fuse:Fuse_1206_3216Metric", S, {"1": "VBUS", "2": "+5V"},
    mpn="Bourns MF-MSMF110-2")
add("D", "Device:D_Schottky", "SS34", "Diode_SMD:D_SMA", S, {"2": "+5V", "1": "VSYS"}, mpn="SS34")
add("J", "Connector_Generic_MountingPin:Conn_01x02_MountingPin", "BATT 2S (6-8.4V)",
    "Connector_JST:JST_PH_B2B-PH-SM4-TB_1x02-1MP_P2.00mm_Vertical", S, {"1": "+BATT", "2": "GND", "MP": "GND"},
    mpn="B2B-PH-SM4-TB(LF)(SN)")
add("D", "Device:D_Schottky", "SS34", "Diode_SMD:D_SMA", S, {"2": "+BATT", "1": "VSYS"}, mpn="SS34")
C(S, "10u", "VSYS", fp=FP_C1206)
C(S, "10u", "+5V", fp=FP_C0805)
B("3.3 V regulator")
add("U", "Regulator_Linear:TLV1117-33", "TLV1117-33", "Package_TO_SOT_SMD:TO-252-3_TabPin2", S,
    {"1": "GND", "2": "+3V3", "3": "VSYS"}, mpn="TLV1117-33IKVURG3")
C(S, "22u", "+3V3", fp=FP_C1206)
C(S, "10u", "+3V3", fp=FP_C0805)
B("1.8 V regulator")
add("U", "Regulator_Linear:AP2112K-1.8", "AP2112K-1.8", "Package_TO_SOT_SMD:SOT-23-5", S,
    {"1": "+3V3", "3": "+3V3", "2": "GND", "5": "+1V8", "4": None}, mpn="AP2112K-1.8TRG1")
C(S, "1u", "+3V3")
C(S, "1u", "+1V8")
B("Voltage sense")
# sense dividers
R(S, "100k", "+BATT", "VBAT_SENSE")
R(S, "33k", "VBAT_SENSE", "GND")
C(S, "100n", "VBAT_SENSE")
R(S, "10k", "VBUS", "VBUS_SENSE")
R(S, "20k", "VBUS_SENSE", "GND")
B("Power LED")
# power LED
R(S, "2.2k", "+3V3", "PWR_LED_K")
add("D", "Device:LED", "LED red", "LED_SMD:LED_0603_1608Metric", S, {"1": "GND", "2": "PWR_LED_K"})
B("Test points")
for n in ("VSYS", "+5V", "+3V3", "+1V8", "+BATT", "GND"):
    TP(S, n)
B("Mounting holes")
for i in range(4):
    add("H", "Mechanical:MountingHole_Pad", "M3", "MountingHole:MountingHole_3.2mm_M3_Pad_Via", S, {"1": "GND"})
B("Power flags")
# PWR_FLAGs
for n in ("GND", "VBUS", "+5V", "VSYS", "+BATT", "VDDA", "VCAP"):
    add("#FLG", "power:PWR_FLAG", "PWR_FLAG", "", S, {"1": n})

# =============================================================== MEMORY
S = "Memory"
B("SDRAM framebuffer")
sd = {"1": "+3V3", "14": "+3V3", "27": "+3V3", "3": "+3V3", "9": "+3V3", "43": "+3V3", "49": "+3V3",
      "28": "GND", "41": "GND", "54": "GND", "6": "GND", "12": "GND", "46": "GND", "52": "GND",
      "15": "FMC_NBL0", "39": "FMC_NBL1", "16": "FMC_SDNWE", "17": "FMC_SDNCAS", "18": "FMC_SDNRAS", "19": "FMC_SDNE0",
      "20": "FMC_BA0", "21": "FMC_BA1", "37": "FMC_SDCKE0", "38": "FMC_SDCLK", "40": None}
for name, pin in zip([f"A{i}" for i in range(13)], ["23", "24", "25", "26", "29", "30", "31", "32", "33", "34", "22", "35", "36"]):
    sd[pin] = "FMC_" + name
for i, pin in enumerate(["2", "4", "5", "7", "8", "10", "11", "13", "42", "44", "45", "47", "48", "50", "51", "53"]):
    sd[pin] = f"FMC_D{i}"
U_SDRAM = add("U", "Memory_RAM:MT48LC16M16A2P", "AS4C4M16SA-7TCN", "Package_SO:TSOP-II-54_22.2x10.16mm_P0.8mm", S, sd,
              mpn="AS4C4M16SA-7TCN (alt MT48LC16M16A2P-6A:G, A12 on pin 36)")
for _ in range(7):
    C(S, "100n", "+3V3").group = ("decap", U_SDRAM.ref)
C(S, "10u", "+3V3", fp=FP_C0805).group = ("decap", U_SDRAM.ref)
B("microSD - SDMMC2")
add("J", "Connector:Micro_SD_Card_Det2", "microSD", "Connector_Card:microSD_HC_Molex_104031-0811", S,
    {"1": "SDMMC2_D2", "2": "SDMMC2_D3", "3": "SDMMC2_CMD", "4": "+3V3", "5": "SDMMC2_CK", "6": "GND",
     "7": "SDMMC2_D0", "8": "SDMMC2_D1", "9": "GND", "10": "SD_DETECT", "11": "GND"}, mpn="1040310811")
for n in ("SDMMC2_CMD", "SDMMC2_D0", "SDMMC2_D1", "SDMMC2_D2", "SDMMC2_D3", "SD_DETECT"):
    R(S, "10k", "+3V3", n)
R(S, "22R", "SDMMC2_CK", "SDMMC2_CK_SD").pins = {"1": "SDMMC2_CK", "2": "SDMMC2_CK_SD"}
# route clock through series resistor at the socket
for c in COMPS:
    if c.lib_id == "Connector:Micro_SD_Card_Det2":
        c.pins["5"] = "SDMMC2_CK_SD"
C(S, "10u", "+3V3", fp=FP_C0805)
C(S, "100n", "+3V3")

# =============================================================== DISPLAY
S = "Display"
B("TFT connector")
fpc = {"1": "BL_LED_K", "2": "BL_VOUT", "3": "GND", "4": "+3V3", "29": "GND", "30": "LTDC_CLK_LCD", "31": "LCD_STBYB",
       "32": "LTDC_HSYNC", "33": "LTDC_VSYNC", "34": "LTDC_DE", "35": None, "36": "GND", "37": None, "38": None,
       "39": None, "40": None, "MP": "GND"}
for i in range(8):
    fpc[str(5 + i)] = f"LTDC_R{i}"
    fpc[str(13 + i)] = f"LTDC_G{i}"
    fpc[str(21 + i)] = f"LTDC_B{i}"
add("J", "Connector_Generic_MountingPin:Conn_01x40_MountingPin", "NHD-5.0-800480TF TFT",
    "Connector_FFC-FPC:Hirose_FH12-40S-0.5SH_1x40-1MP_P0.50mm_Horizontal", S, fpc,
    mpn="FH12-40S-0.5SH(55) (display spec lists Molex 54104-4031 - check contact side)")
R(S, "33R", "LTDC_CLK", "LTDC_CLK_LCD")
R(S, "10k", "LCD_STBYB", "GND")
C(S, "10u", "+3V3", fp=FP_C0805)
C(S, "100n", "+3V3")
B("Capacitive touch")
add("J", "Connector_Generic_MountingPin:Conn_01x06_MountingPin", "CTP touch FFC",
    "Connector_FFC-FPC:Molex_200528-0060_1x06-1MP_P1.00mm_Horizontal", S,
    {"1": "+3V3", "2": "GND", "3": "I2C1_SCL", "4": "I2C1_SDA", "5": "TOUCH_INT_N", "6": "TOUCH_RST_N", "MP": "GND"},
    mpn="Molex 200528-0060 (display spec lists 52271-0679)")
R(S, "10k", "+3V3", "TOUCH_INT_N")
R(S, "10k", "+3V3", "TOUCH_RST_N")
B("Backlight driver")
# backlight boost (TPS61165, ~29 mA string current)
add("U", "Driver_LED:TPS61165DBV", "TPS61165", "Package_TO_SOT_SMD:SOT-23-6", S,
    {"1": "VSYS", "2": "BL_PWM", "3": "BL_SW", "4": "GND", "5": "BL_COMP", "6": "BL_LED_K"}, mpn="TPS61165DBVR")
add("L", "Device:L", "10u", "Inductor_SMD:L_Bourns-SRN4018", S, {"1": "VSYS", "2": "BL_SW"}, mpn="SRN4018-100M")
add("D", "Device:D_Schottky", "B0540W", "Diode_SMD:D_SOD-123", S, {"2": "BL_SW", "1": "BL_VOUT"}, mpn="B0540W-7-F")
C(S, "4.7u", "VSYS", fp=FP_C0805)
C(S, "1u/50V", "BL_VOUT", fp=FP_C0805)
C(S, "220n", "BL_COMP")
R(S, "6.8R", "BL_LED_K", "GND")
R(S, "100k", "BL_PWM", "GND")

# =============================================================== SENSORS
S = "Sensors"
B("IMU - ICM-20948 (1.8 V)")
add("U", "Sensor_Motion:ICM-20948", "ICM-20948", "Sensor_Motion:InvenSense_QFN-24_3x3mm_P0.4mm", S,
    {"8": "+1V8", "13": "+1V8", "18": "GND", "20": "GND", "10": "ICM_REGOUT", "11": "GND", "12": "IMU_INT_1V8",
     "9": "+1V8", "22": "+1V8", "23": "I2C_1V8_SCL", "24": "I2C_1V8_SDA", "21": None, "7": None, "19": None,
     **{str(n): None for n in (1, 2, 3, 4, 5, 6, 14, 15, 16, 17)}}, mpn="ICM-20948")
C(S, "100n", "ICM_REGOUT")
C(S, "100n", "+1V8")
C(S, "100n", "+1V8")
B("Barometer - BMP390")
add("U", "moVe:BMP390", "BMP390", "moVe:Bosch_LGA-10_2x2mm_P0.5mm", S,
    {"10": "+3V3", "1": "+3V3", "2": "I2C1_SCL", "4": "I2C1_SDA", "5": "GND", "6": "+3V3", "7": "BARO_INT",
     "3": "GND", "8": "GND", "9": "GND"}, mpn="BMP390")
C(S, "100n", "+3V3")
C(S, "100n", "+3V3")
B("I2C level shift 3.3 V / 1.8 V")
add("U", "Interface:PCA9306", "PCA9306DCT", "Package_SO:SSOP-8_2.95x2.8mm_P0.65mm", S,
    {"1": "GND", "2": "+1V8", "3": "I2C_1V8_SCL", "4": "I2C_1V8_SDA", "5": "I2C1_SDA", "6": "I2C1_SCL",
     "7": "LS_VREF2", "8": "LS_VREF2"}, mpn="PCA9306DCTR")
R(S, "200k", "+3V3", "LS_VREF2")
C(S, "100n", "LS_VREF2")
R(S, "4.7k", "+3V3", "I2C1_SCL")
R(S, "4.7k", "+3V3", "I2C1_SDA")
R(S, "4.7k", "+1V8", "I2C_1V8_SCL")
R(S, "4.7k", "+1V8", "I2C_1V8_SDA")
B("IMU INT level shift")
# IMU INT 1.8V -> 3.3V shifter
add("Q", "Transistor_FET:BSS138", "BSS138", "Package_TO_SOT_SMD:SOT-23", S,
    {"1": "+1V8", "2": "IMU_INT_1V8", "3": "IMU_INT"}, mpn="BSS138")
R(S, "10k", "+1V8", "IMU_INT_1V8")
R(S, "10k", "+3V3", "IMU_INT")

# =============================================================== RADIO
S = "Radio"
B("GNSS - MAX-M10S")
add("U", "RF_GPS:MAX-M10S", "MAX-M10S", "RF_GPS:ublox_MAX", S,
    {"1": "GND", "10": "GND", "12": "GND", "11": "GPS_RF_IN", "13": None, "14": None, "15": None,
     "16": None, "17": None, "18": "GPS_SAFEBOOT_N", "2": "GPS_UART_RX", "3": "GPS_UART_TX",
     "4": "GPS_TIMEPULSE", "5": "GPS_EXTINT", "6": "+3V3", "7": "+3V3", "8": "+3V3", "9": "GPS_RESET_N"},
    mpn="MAX-M10S-00B")
C(S, "100n", "+3V3")
C(S, "10u", "+3V3", fp=FP_C0805)
add("J", "Connector:Conn_Coaxial", "GNSS ANT", "Connector_Coaxial:U.FL_Hirose_U.FL-R-SMT-1_Vertical", S,
    {"1": "GPS_RF_IN", "2": "GND"}, mpn="U.FL-R-SMT-1(10)")
add("TP", "Connector:TestPoint", "GPS_SAFEBOOT", "TestPoint:TestPoint_Pad_D1.5mm", S, {"1": "GPS_SAFEBOOT_N"})
B("Bluetooth LE - NORA-B261")
nora = {"A7": "+3V3", "A8": "+3V3", "B7": "+3V3", "G9": "BLE_UART_TX", "G8": "BLE_UART_RX",
        "F9": "BLE_RTS", "F8": "BLE_CTS", "J3": "BLE_RESET_N", "D8": "BLE_SW1", "C8": "BLE_SW2",
        "J9": "BLE_WAKE", "E8": None, "E9": None, "K9": "BLE_ANT", "A5": "BLE_DBG_TX", "J8": None,
        "H8": None, "H9": None, "C5": None, "B5": None}
from customlib import NORA_GND
for g in NORA_GND:
    nora[g] = "GND"
add("U", "moVe:NORA-B261", "NORA-B261", "moVe:ublox_NORA-B2x1", S, nora, mpn="NORA-B261-00B")
C(S, "100n", "+3V3")
C(S, "10u", "+3V3", fp=FP_C0805)
# pi match (series 0R fitted, shunts DNP)
C(S, "DNP", "BLE_ANT", "GND", fp="Capacitor_SMD:C_0402_1005Metric", dnp=True)
R(S, "0R", "BLE_ANT", "BLE_ANT_M", fp="Resistor_SMD:R_0402_1005Metric")
C(S, "DNP", "BLE_ANT_M", "GND", fp="Capacitor_SMD:C_0402_1005Metric", dnp=True)
add("J", "Connector:Conn_Coaxial", "BLE ANT", "Connector_Coaxial:U.FL_Hirose_U.FL-R-SMT-1_Vertical", S,
    {"1": "BLE_ANT_M", "2": "GND"}, mpn="U.FL-R-SMT-1(10)")
add("TP", "Connector:TestPoint", "BLE_SW1", "TestPoint:TestPoint_Pad_D1.5mm", S, {"1": "BLE_SW1"})
add("TP", "Connector:TestPoint", "BLE_DBG_TX", "TestPoint:TestPoint_Pad_D1.5mm", S, {"1": "BLE_DBG_TX"})
add("TP", "Connector:TestPoint", "BLE_RESET", "TestPoint:TestPoint_Pad_D1.5mm", S, {"1": "BLE_RESET_N"})

# =============================================================== CO SENSOR (aircraft variant)
S = "CO_Sensor"
B("MQ-7 heater and sense")
add("U", "Sensor_Gas:MQ-6", "MQ-7B", "Sensor:MQ-6", S,
    {"6": "+5V", "4": "+5V", "1": "MQ7_OUT", "3": "MQ7_OUT", "2": "+5V", "5": "MQ7_HEAT_N"},
    mpn="Winsen MQ-7B (aircraft build only)")
add("Q", "Transistor_FET:AO3400A", "AO3400A", "Package_TO_SOT_SMD:SOT-23", S,
    {"1": "MQ7_GATE", "2": "GND", "3": "MQ7_HEAT_N"}, mpn="AO3400A")
R(S, "100R", "MQ7_HEAT", "MQ7_GATE")
R(S, "100k", "MQ7_GATE", "GND")
R(S, "10k", "MQ7_OUT", "GND")          # RL load resistor
R(S, "10k", "MQ7_OUT", "MQ7_ADC")     # divider top
R(S, "20k", "MQ7_ADC", "GND")         # divider bottom -> 3.33V max
C(S, "100n", "MQ7_ADC")
C(S, "10u", "+5V", fp=FP_C0805)

SHEETS = ["Power", "MCU", "Memory", "Display", "Sensors", "Radio", "CO_Sensor"]
SHEET_TITLES = {"Power": "Power input, regulation, mechanical", "MCU": "STM32H723ZGT6 core",
                "Memory": "SDRAM framebuffer + microSD", "Display": "RGB TFT, touch, backlight",
                "Sensors": "IMU + barometer", "Radio": "GNSS + Bluetooth LE",
                "CO_Sensor": "MQ-7 CO sensor (aircraft variant)"}
POWER_NETS = {"GND": "power:GND", "+3V3": "power:+3V3", "+1V8": "power:+1V8", "+5V": "power:+5V",
              "VBUS": "power:VBUS", "+BATT": "power:+BATT", "VSYS": "moVe:VSYS", "VDDA": "moVe:VDDA",
              "VCAP": "moVe:VCAP"}
