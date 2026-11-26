"""Project-local symbols (moVe.kicad_sym) and footprints (moVe.pretty)."""
import os
from sexp import Sym as S, dump, q

def _pin(num, name, x, y, ang, typ="passive", length=2.54, hide=False):
    p = [S("pin"), S(typ), S("line"), [S("at"), x, y, ang], [S("length"), length]]
    if hide:
        p.append(S("hide"))
    p += [[S("name"), name, [S("effects"), [S("font"), [S("size"), 1.27, 1.27]]]],
          [S("number"), num, [S("effects"), [S("font"), [S("size"), 1.27, 1.27]]]]]
    return p

def _prop(k, v, x, y, hide=False):
    eff = [S("effects"), [S("font"), [S("size"), 1.27, 1.27]]]
    if hide:
        eff.append(S("hide"))
    return [S("property"), k, v, [S("at"), x, y, 0], eff]

def box_symbol(name, ref, value, footprint, left, right, width=20.32, datasheet="~", desc=""):
    """left/right: lists of (number, name, type) or None for spacer; drawn top to bottom."""
    n = max(len(left), len(right))
    h = (n + 1) * 2.54
    top = round(h / 2 / 2.54) * 2.54
    w2 = width / 2
    body = [S("symbol"), name + "_0_1",
            [S("rectangle"), [S("start"), -w2, top], [S("end"), w2, top - (n + 1) * 2.54],
             [S("stroke"), [S("width"), 0.254], [S("type"), S("default")]],
             [S("fill"), [S("type"), S("background")]]]]
    pins = [S("symbol"), name + "_1_1"]
    for side, lst in (("L", left), ("R", right)):
        for i, item in enumerate(lst):
            if item is None:
                continue
            num, pname, typ = item
            y = top - (i + 1) * 2.54
            if side == "L":
                pins.append(_pin(num, pname, -w2 - 5.08, y, 0, typ, 5.08))
            else:
                pins.append(_pin(num, pname, w2 + 5.08, y, 180, typ, 5.08))
    return [S("symbol"), name, [S("in_bom"), S("yes")], [S("on_board"), S("yes")],
            _prop("Reference", ref, 0, top + 2.54),
            _prop("Value", value, 0, top - (n + 1) * 2.54 - 2.54),
            _prop("Footprint", footprint, 0, 0, True),
            _prop("Datasheet", datasheet, 0, 0, True),
            _prop("Description", desc, 0, 0, True),
            body, pins]

def power_symbol(name):
    """Global power symbol (arrow up), net named `name`."""
    return [S("symbol"), name, [S("power")], [S("pin_names"), [S("offset"), 0]],
            [S("in_bom"), S("yes")], [S("on_board"), S("yes")],
            _prop("Reference", "#PWR", 0, -3.81, True),
            _prop("Value", name, 0, 3.556),
            _prop("Footprint", "", 0, 0, True),
            _prop("Datasheet", "", 0, 0, True),
            [S("symbol"), name + "_0_1",
             [S("polyline"), [S("pts"), [S("xy"), -0.762, 1.27], [S("xy"), 0, 2.54]],
              [S("stroke"), [S("width"), 0], [S("type"), S("default")]], [S("fill"), [S("type"), S("none")]]],
             [S("polyline"), [S("pts"), [S("xy"), 0, 0], [S("xy"), 0, 2.54]],
              [S("stroke"), [S("width"), 0], [S("type"), S("default")]], [S("fill"), [S("type"), S("none")]]],
             [S("polyline"), [S("pts"), [S("xy"), 0, 2.54], [S("xy"), 0.762, 1.27]],
              [S("stroke"), [S("width"), 0], [S("type"), S("default")]], [S("fill"), [S("type"), S("none")]]]],
            [S("symbol"), name + "_1_1", _pin("1", name, 0, 0, 90, "power_in", 0, hide=True)]]

PW, PI, PO, BI, IN, OUT, NC = "power_in", "power_in", "power_out", "bidirectional", "input", "output", "no_connect"

NORA_GND = ["A1", "A9", "B2", "B8", "E4", "E5", "F4", "F5", "K2", "K5", "K8"]

def custom_symbols():
    syms = []
    syms.append(box_symbol(
        "BMP390", "U", "BMP390", "moVe:Bosch_LGA-10_2x2mm_P0.5mm",
        left=[("10", "VDD", PI), ("1", "VDDIO", PI), None, ("2", "SCK", IN), ("4", "SDI", BI),
              ("5", "SDO", BI), ("6", "CSB", IN)],
        right=[("7", "INT", OUT), None, None, None, ("3", "VSS", PI), ("8", "VSS", PI), ("9", "VSS", PI)],
        width=15.24, datasheet="https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp390/",
        desc="Barometric pressure sensor, I2C/SPI, LGA-10 2x2mm"))
    left = [("A7", "VCC", PI), ("A8", "VCC", PI), ("B7", "VCC", PI), None,
            ("G9", "UART_RXD", IN), ("G8", "UART_TXD", OUT), ("F9", "UART_CTS", IN), ("F8", "UART_RTS", OUT), None,
            ("J3", "~{RESET}", IN), ("D8", "SWITCH_1", IN), ("C8", "SWITCH_2", IN), ("J9", "GPIO_J9/WAKE", BI),
            ("E8", "GPIO_E8", BI), ("E9", "GPIO_E9", BI)]
    right = [("K9", "ANT", BI), None, ("A5", "DBG_TX", OUT), ("J8", "RED", OUT), ("H8", "GREEN", OUT),
             ("H9", "BLUE", OUT), ("C5", "NFC1", "passive"), ("B5", "NFC2", "passive"), None]
    right += [(g, "GND", PI) for g in NORA_GND]
    syms.append(box_symbol("NORA-B261", "U", "NORA-B261-00B", "moVe:ublox_NORA-B2x1",
                           left, right, width=22.86,
                           datasheet="https://content.u-blox.com/sites/default/files/documents/NORA-B2-u-connectXpress_DataSheet_UBXDOC-465451970-4608.pdf",
                           desc="u-blox NORA-B261 u-connectXpress Bluetooth LE module, antenna pin"))
    for p in ["VSYS", "VDDA", "VCAP"]:
        syms.append(power_symbol(p))
    return syms

def write_symbol_lib(path):
    tree = [S("kicad_symbol_lib"), [S("version"), 20220914], [S("generator"), S("moVe_gen")]] + custom_symbols()
    open(path, "w").write(dump(tree) + "\n")

# ---------------------------------------------------------------- footprints
def _fp_pad(num, typ, shape, x, y, w, h, layers, rr=None, drill=None):
    p = [S("pad"), num, S(typ), S(shape), [S("at"), x, y], [S("size"), w, h]]
    if drill:
        p.append([S("drill"), drill])
    p.append([S("layers")] + layers)
    if rr is not None:
        p.append([S("roundrect_rratio"), rr])
    return p

def _fp_line(x1, y1, x2, y2, layer, w=0.12):
    return [S("fp_line"), [S("start"), x1, y1], [S("end"), x2, y2],
            [S("stroke"), [S("width"), w], [S("type"), S("solid")]], [S("layer"), layer]]

def _fp_rect(x1, y1, x2, y2, layer, w=0.05):
    return [_fp_line(x1, y1, x2, y1, layer, w), _fp_line(x2, y1, x2, y2, layer, w),
            _fp_line(x2, y2, x1, y2, layer, w), _fp_line(x1, y2, x1, y1, layer, w)]

def _fp_text(kind, text, x, y, layer, hide=False):
    t = [S("fp_text"), S(kind), text, [S("at"), x, y], [S("layer"), layer]]
    if hide:
        t.append(S("hide"))
    t.append([S("effects"), [S("font"), [S("size"), 1, 1], [S("thickness"), 0.15]]])
    return t

SMD = [S("F.Cu"), S("F.Paste"), S("F.Mask")]

def fp_nora():
    """NORA-B2x1 land pattern per data sheet Fig.5 (UBXDOC-465451970-4608 R04): 0.60mm sq pads, 1.10mm pitch."""
    rows = "ABCDEFGHJK"  # A bottom (y=+4.9) ... K top (y=-5.0)
    present = {
        "A": [9, 8, 7, 6, 5, 4, 3, 2, 1], "B": [9, 8, 7, 6, 5, 4, 3, 2, 1],
        "C": [9, 8, 6, 5, 4, 2, 1], "D": [9, 8, 7, 3, 2, 1], "E": [9, 8, 7, 5, 4, 3, 2, 1],
        "F": [9, 8, 7, 5, 4, 3, 2, 1], "G": [9, 8, 7, 5, 4, 3, 2, 1], "H": [9, 8, 7, 3, 2, 1],
        "J": [9, 8, 7, 5, 4, 3, 2, 1], "K": [9, 8, 5, 2, 1]}
    items = []
    for r, row in enumerate(rows):
        y = round(4.9 - 1.1 * r, 3)
        for c in present[row]:
            if row in "EFGJ" and c in (5, 4):
                x = -0.55 if c == 5 else 0.55
            elif row == "C" and c in (6, 5, 4):
                x = {6: -1.1, 5: 0.0, 4: 1.1}[c]
            elif row == "K" and c == 5:
                x = 0.0
            else:
                x = round((5 - c) * 1.1, 3)
            items.append(_fp_pad(f"{row}{c}", "smd", "rect", x, y, 0.6, 0.6, SMD))
    out = [S("footprint"), "ublox_NORA-B2x1", [S("version"), 20221018], [S("generator"), S("moVe_gen")],
           [S("layer"), "F.Cu"],
           [S("descr"), "u-blox NORA-B2x1 (antenna pin) module 10.4x11.2mm, land pattern per NORA-B2 u-connectXpress data sheet Figure 5. Pads K3/K7 shown in Fig.2 but absent in Fig.5 - verify."],
           [S("attr"), S("smd")],
           _fp_text("reference", "REF**", 0, -6.6, "F.SilkS"),
           _fp_text("value", "NORA-B261", 0, 6.7, "F.Fab")]
    out += _fp_rect(-5.2, -5.6, 5.2, 5.6, "F.Fab", 0.1)
    out += _fp_rect(-5.45, -5.85, 5.45, 5.85, "F.CrtYd", 0.05)
    out += [_fp_line(-5.32, -5.72, 5.32, -5.72, "F.SilkS"), _fp_line(-5.32, 5.72, 5.32, 5.72, "F.SilkS"),
            _fp_line(-5.32, -5.72, -5.32, 5.72, "F.SilkS"), _fp_line(5.32, -5.72, 5.32, 5.72, "F.SilkS"),
            [S("fp_circle"), [S("center"), 5.0, 6.15], [S("end"), 5.2, 6.15],
             [S("stroke"), [S("width"), 0.2], [S("type"), S("solid")]], [S("fill"), S("solid")], [S("layer"), "F.SilkS"]]]
    out += items
    return out

def fp_bmp390():
    """Bosch BMP390 LGA-10 2.0x2.0 mm, top view, from BMP390 data sheet Fig.23/26 (pads extended 0.05mm outward)."""
    pads = []
    # top row: pin2 (x=-0.25), pin1 (x=+0.25); bottom: pin6 (-0.25), pin7 (+0.25)
    for num, x, y in [("1", 0.25, -0.7875), ("2", -0.25, -0.7875), ("6", -0.25, 0.7875), ("7", 0.25, 0.7875)]:
        pads.append(_fp_pad(num, "smd", "rect", x, y, 0.25, 0.325, SMD))
    for num, x, y in [("3", -0.7875, -0.5), ("4", -0.7875, 0.0), ("5", -0.7875, 0.5),
                      ("10", 0.7875, -0.5), ("9", 0.7875, 0.0), ("8", 0.7875, 0.5)]:
        pads.append(_fp_pad(num, "smd", "rect", x, y, 0.325, 0.25, SMD))
    out = [S("footprint"), "Bosch_LGA-10_2x2mm_P0.5mm", [S("version"), 20221018], [S("generator"), S("moVe_gen")],
           [S("layer"), "F.Cu"], [S("descr"), "Bosch BMP390 LGA-10 2x2x0.75mm, per BST-BMP390-DS002 Fig. 23/26"],
           [S("attr"), S("smd")],
           _fp_text("reference", "REF**", 0, -1.8, "F.SilkS"), _fp_text("value", "BMP390", 0, 1.9, "F.Fab")]
    out += _fp_rect(-1.0, -1.0, 1.0, 1.0, "F.Fab", 0.1)
    out += _fp_rect(-1.35, -1.35, 1.35, 1.35, "F.CrtYd", 0.05)
    out += [_fp_line(-1.11, -1.11, -0.5, -1.11, "F.SilkS"), _fp_line(1.11, 1.11, 0.5, 1.11, "F.SilkS"),
            _fp_line(-1.11, 1.11, -0.5, 1.11, "F.SilkS"),
            [S("fp_circle"), [S("center"), 0.95, -1.3], [S("end"), 1.05, -1.3],
             [S("stroke"), [S("width"), 0.15], [S("type"), S("solid")]], [S("fill"), S("solid")], [S("layer"), "F.SilkS"]]]
    return out + pads

def write_footprint_lib(path):
    os.makedirs(path, exist_ok=True)
    for fp in (fp_nora(), fp_bmp390()):
        open(os.path.join(path, fp[1] + ".kicad_mod"), "w").write(dump(fp) + "\n")
