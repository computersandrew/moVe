"""Net classes -> DSN -> Freerouting -> SES import -> pours -> zone fill -> DRC."""
import json, math, os, subprocess, sys, time
import pcbnew
from pcbnew import FromMM, ToMM

OUT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PCB = os.path.join(OUT, "moVe_test.kicad_pcb")
PRO = os.path.join(OUT, "moVe_test.kicad_pro")
STAGE = sys.argv[1] if len(sys.argv) > 1 else "all"
PASSES = int(os.environ.get("PASSES", "40"))

def nc(name, clr, w, via=0.6, drill=0.3):
    return {"name": name, "clearance": clr, "track_width": w, "via_diameter": via, "via_drill": drill,
            "microvia_diameter": 0.3, "microvia_drill": 0.1, "diff_pair_width": 0.2, "diff_pair_gap": 0.25,
            "diff_pair_via_gap": 0.25, "wire_width": 6, "bus_width": 12, "line_style": 0,
            "pcb_color": "rgba(0, 0, 0, 0.000)", "schematic_color": "rgba(0, 0, 0, 0.000)"}

POWER = ["VSYS", "+5V", "+BATT"]
POWER_N = ["VBUS", "BL_SW", "MQ7_HEAT_N"]
RF = ["GPS_RF_IN", "BLE_ANT", "BLE_ANT_M"]

def write_project():
    pro = {
        "meta": {"filename": "moVe_test.kicad_pro", "version": 1},
        "board": {"design_settings": {
            "defaults": {"board_outline_line_width": 0.1, "copper_line_width": 0.2, "copper_text_size_h": 1.5,
                         "copper_text_size_v": 1.5, "copper_text_thickness": 0.3, "silk_line_width": 0.12,
                         "silk_text_size_h": 0.8, "silk_text_size_v": 0.8, "silk_text_thickness": 0.12},
            "rules": {"min_clearance": 0.127, "min_track_width": 0.127, "min_via_diameter": 0.45,
                      "min_via_annular_width": 0.1, "min_through_hole_diameter": 0.2, "min_hole_clearance": 0.19,
                      "min_hole_to_hole": 0.25, "min_copper_edge_clearance": 0.3, "min_microvia_diameter": 0.2,
                      "min_microvia_drill": 0.1, "min_silk_clearance": 0.0, "min_text_height": 0.6,
                      "min_text_thickness": 0.08, "solder_mask_clearance": 0.0, "solder_mask_min_width": 0.0},
            "track_widths": [0.0, 0.15, 0.25, 0.36, 0.4, 0.6, 1.0],
            "via_dimensions": [{"diameter": 0.0, "drill": 0.0}, {"diameter": 0.6, "drill": 0.3}],
        }},
        "net_settings": {
            "classes": [nc("Default", 0.15, 0.15), nc("Power", 0.2, 0.5, 0.8, 0.4), nc("RF", 0.3, 0.36), nc("PowerNarrow", 0.15, 0.3),
                        nc("USB", 0.15, 0.25)],
            "meta": {"version": 3}, "net_colors": None, "netclass_assignments": None,
            "netclass_patterns": [{"netclass": "Power", "pattern": n} for n in POWER] +
                                 [{"netclass": "RF", "pattern": n} for n in RF] +
                                 [{"netclass": "PowerNarrow", "pattern": n} for n in POWER_N] +
                                 [{"netclass": "USB", "pattern": n} for n in ("USB_DP", "USB_DM", "USB_C_DP", "USB_C_DM")],
        },
        "schematic": {"legacy_lib_dir": "", "legacy_lib_list": []},
        "sheets": [], "text_variables": {},
    }
    json.dump(pro, open(PRO, "w"), indent=2)

if STAGE in ("all", "dsn"):
    write_project()
    b = pcbnew.LoadBoard(PCB)
    for n in ("VSYS", "GPS_RF_IN", "LTDC_R0", "USB_DP"):
        ni = b.FindNet(n)
        print(n, ni.GetNetClassName())
    ok = pcbnew.ExportSpecctraDSN(b, os.path.join(OUT, "moVe_test.dsn"))
    print("dsn", ok)

if STAGE in ("all", "route"):
    t = time.time()
    r = subprocess.run(["xvfb-run", "-a", "java", "-Djava.awt.headless=false", "-jar",
                        os.environ.get("FREEROUTING_JAR", "freerouting.jar"), "-de", os.path.join(OUT, "moVe_test.dsn"),
                        "-do", os.path.join(OUT, "moVe_test.ses"), "-mp", str(PASSES), "-mt", "1"],
                       capture_output=True, text=True, timeout=5400)
    print("freerouting rc", r.returncode, "%.0fs" % (time.time() - t))
    print(r.stdout[-3000:], r.stderr[-2000:])

def circle_poly(cx, cy, r, n=128):
    chain = pcbnew.SHAPE_LINE_CHAIN()
    for i in range(n):
        a = 2 * math.pi * i / n
        chain.Append(FromMM(cx + r * math.cos(a)), FromMM(cy + r * math.sin(a)))
    chain.SetClosed(True)
    return chain

if STAGE in ("all", "import"):
    b = pcbnew.LoadBoard(PCB)
    ses = os.path.join(OUT, "moVe_test.ses")
    if os.path.exists(ses):
        print("ses import", pcbnew.ImportSpecctraSES(b, ses))
    gnd = b.FindNet("GND")
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        z = pcbnew.ZONE(b)
        z.SetLayer(layer); z.SetNet(gnd)
        z.Outline().AddOutline(circle_poly(150, 100, 4.2 * 25.4 / 2 - 0.5))
        z.SetLocalClearance(FromMM(0.3)); z.SetMinThickness(FromMM(0.2))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        z.SetThermalReliefGap(FromMM(0.3)); z.SetThermalReliefSpokeWidth(FromMM(0.4))
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        b.Add(z)
    filler = pcbnew.ZONE_FILLER(b)
    filler.Fill(b.Zones())
    b.Save(PCB)
    print("saved")

if STAGE in ("all", "import", "drc"):
    b = pcbnew.LoadBoard(PCB)
    rpt = os.path.join(OUT, "moVe_test-drc.rpt")
    pcbnew.WriteDRCReport(b, rpt, pcbnew.EDA_UNITS_MILLIMETRES, True)
    print(open(rpt).read()[:300])
