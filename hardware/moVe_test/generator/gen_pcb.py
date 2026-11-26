"""Build the 4.2in round 4-layer moVe test PCB from the schematic netlist (KiCad 7 pcbnew API)."""
import math, os, re, sys, json
import pcbnew
from pcbnew import VECTOR2I, FromMM, ToMM
from sexp import parse, find, findall
import design as D

OUT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
NETFILE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(OUT, "moVe_test.net")
BOARD = os.environ.get("BOARD_OUT", os.path.join(OUT, "moVe_test.kicad_pcb"))
CX, CY = 150.0, 100.0
RAD = 4.2 * 25.4 / 2          # 53.34 mm
KEEP_R = RAD - 1.2            # component keep-in radius

def P(x, y):
    return VECTOR2I(FromMM(x), FromMM(y))

# ---------------------------------------------------------------- netlist
nl = parse(open(NETFILE).read())
comps = {}
for c in findall(find(nl, "components"), "comp"):
    ref = find(c, "ref")[1]
    sp = find(c, "sheetpath")
    comps[ref] = dict(value=find(c, "value")[1], footprint=find(c, "footprint")[1],
                      path=find(sp, "tstamps")[1] + find(c, "tstamps")[1])
pad_net = {}
netnames = []
for n in findall(find(nl, "nets"), "net"):
    name = find(n, "name")[1].lstrip("/")
    netnames.append(name)
    for node in findall(n, "node"):
        pad_net[(find(node, "ref")[1], find(node, "pin")[1])] = name

board = pcbnew.BOARD()
board.SetCopperLayerCount(4)
board.SetLayerType(pcbnew.In1_Cu, pcbnew.LT_POWER)
board.SetLayerType(pcbnew.In2_Cu, pcbnew.LT_POWER)
board.SetLayerName(pcbnew.In1_Cu, "In1.GND")
board.SetLayerName(pcbnew.In2_Cu, "In2.3V3")
nets = {}
for name in netnames:
    ni = pcbnew.NETINFO_ITEM(board, name)
    board.Add(ni)
    nets[name] = ni

FPS = {}
dcomp = {c.ref: c for c in D.COMPS}
for ref, c in comps.items():
    lib, name = c["footprint"].split(":")
    libdir = os.path.join(OUT, "moVe.pretty") if lib == "moVe" else os.path.join(os.environ.get("KICAD7_FOOTPRINT_DIR", "/usr/share/kicad/footprints"), f"{lib}.pretty")
    fp = pcbnew.FootprintLoad(libdir, name)
    if fp is None:
        raise SystemExit(f"footprint not found {c['footprint']}")
    fp.SetFPIDAsString(c["footprint"])
    fp.SetReference(ref)
    fp.SetValue(c["value"])
    fp.SetPath(pcbnew.KIID_PATH(c["path"]))
    for f in (fp.Reference(), fp.Value()):
        f.SetTextSize(VECTOR2I(FromMM(0.8), FromMM(0.8)))
        f.SetTextThickness(FromMM(0.12))
    fp.Value().SetVisible(False)
    board.Add(fp)
    for pad in fp.Pads():
        net = pad_net.get((ref, pad.GetNumber()))
        if net:
            pad.SetNet(nets[net])
    FPS[ref] = fp

# sanity: every netlist node has a pad
for (ref, pin), net in pad_net.items():
    if not any(p.GetNumber() == pin for p in FPS[ref].Pads()):
        raise SystemExit(f"pad {ref}.{pin} missing in footprint")

# ---------------------------------------------------------------- outline
circ = pcbnew.PCB_SHAPE(board)
circ.SetShape(pcbnew.SHAPE_T_CIRCLE)
circ.SetCenter(P(CX, CY))
circ.SetEnd(P(CX + RAD, CY))
circ.SetLayer(pcbnew.Edge_Cuts)
circ.SetWidth(FromMM(0.1))
board.Add(circ)

# ---------------------------------------------------------------- placement helpers
def find_ref(pred):
    out = [c.ref for c in D.COMPS if not c.ref.startswith("#") and pred(c)]
    return out

def one(pred):
    r = find_ref(pred)
    assert len(r) == 1, r
    return r[0]

def rotv(x, y, deg):
    a = math.radians(deg)
    return x * math.cos(a) + y * math.sin(a), -x * math.sin(a) + y * math.cos(a)

placed = {"top": [], "bot": []}

def fp_box(fp, margin=0.3):
    bb = fp.GetBoundingBox(False, False)
    return (ToMM(bb.GetLeft()) - margin, ToMM(bb.GetTop()) - margin,
            ToMM(bb.GetRight()) + margin, ToMM(bb.GetBottom()) + margin)

def is_tht(fp):
    return any(p.GetAttribute() in (pcbnew.PAD_ATTRIB_PTH, pcbnew.PAD_ATTRIB_NPTH) for p in fp.Pads())

def overlaps(box, side):
    for b in placed[side]:
        if box[0] < b[2] and box[2] > b[0] and box[1] < b[3] and box[3] > b[1]:
            return True
    return False

def inside(box, r=KEEP_R):
    return all(math.hypot(x - CX, y - CY) <= r for x in (box[0], box[2]) for y in (box[1], box[3]))

def commit(fp, side):
    box = fp_box(fp)
    sides = ["top", "bot"] if is_tht(fp) else [side]
    for s in sides:
        placed[s].append(box)

def put(ref, x, y, rot=0.0, side="top", check=True):
    fp = FPS[ref]
    if side == "bot" and not fp.IsFlipped():
        fp.Flip(fp.GetPosition(), False)
    fp.SetOrientationDegrees(rot)
    fp.SetPosition(P(x, y))
    box = fp_box(fp)
    if check:
        if not inside(box, RAD - 0.2):
            print(f"WARN {ref} outside keep-in")
        if overlaps(box, side) or (is_tht(fp) and overlaps(box, "bot" if side == "top" else "top")):
            print(f"WARN {ref} overlaps at {x:.1f},{y:.1f}")
    commit(fp, side)
    return fp

def put_edge(ref, angle_deg, local_front, local_edge_pt, inset=0.0, side="top"):
    """Place connector on the rim at polar angle (deg, 0=+x, 90=down) with its local 'front' pointing outward."""
    fp = FPS[ref]
    w = (math.cos(math.radians(angle_deg)), math.sin(math.radians(angle_deg)))
    rot = math.degrees(math.atan2(local_front[1], local_front[0]) - math.atan2(w[1], w[0]))
    rot = (rot + 360) % 360
    ex, ey = rotv(local_edge_pt[0], local_edge_pt[1], rot)
    R = RAD - inset
    x, y = CX + w[0] * R - ex, CY + w[1] * R - ey
    return put(ref, x, y, rot, side)

def pad_pos(ref, num):
    for p in FPS[ref].Pads():
        if p.GetNumber() == num:
            v = p.GetPosition()
            return ToMM(v.x), ToMM(v.y)
    raise KeyError((ref, num))

def try_place(ref, tx, ty, side="top", rot=0.0, maxr=30.0, keep_r=KEEP_R):
    fp = FPS[ref]
    if side == "bot" and not fp.IsFlipped():
        fp.Flip(fp.GetPosition(), False)
    for rr in [0.0] + [0.5 * i for i in range(1, int(maxr / 0.5))]:
        n = max(1, int(2 * math.pi * rr / 0.6))
        start = math.atan2(ty - CY, tx - CX)
        for k in range(n):
            a = start + 2 * math.pi * k / n
            x, y = tx + rr * math.cos(a), ty + rr * math.sin(a)
            for rt in (rot, rot + 90):
                fp.SetOrientationDegrees(rt)
                fp.SetPosition(P(round(x, 2), round(y, 2)))
                box = fp_box(fp)
                if not inside(box, keep_r):
                    continue
                if overlaps(box, side) or (is_tht(fp) and overlaps(box, "bot" if side == "top" else "top")):
                    continue
                commit(fp, side)
                return True
    print("FAILED to place", ref)
    return False

# ---------------------------------------------------------------- key parts
MCU = one(lambda c: c.lib_id.startswith("MCU_ST"))
SDRAM = one(lambda c: c.lib_id.startswith("Memory_RAM"))
FPC = one(lambda c: "Conn_01x40" in c.lib_id)
TOUCH = one(lambda c: "Conn_01x06" in c.lib_id)
USB = one(lambda c: "USB_C" in c.lib_id)
SD = one(lambda c: "Micro_SD" in c.lib_id)
GPS = one(lambda c: c.lib_id == "RF_GPS:MAX-M10S")
NORA = one(lambda c: c.lib_id == "moVe:NORA-B261")
GPS_UFL = one(lambda c: c.value == "GNSS ANT")
BLE_UFL = one(lambda c: c.value == "BLE ANT")
IMU = one(lambda c: c.lib_id == "Sensor_Motion:ICM-20948")
BARO = one(lambda c: c.lib_id == "moVe:BMP390")
LS = one(lambda c: c.lib_id == "Interface:PCA9306")
MQ = one(lambda c: c.lib_id == "Sensor_Gas:MQ-6")
LDO33 = one(lambda c: c.value == "TLV1117-33")
LDO18 = one(lambda c: c.value == "AP2112K-1.8")
BOOST = one(lambda c: c.value == "TPS61165")
SWD = one(lambda c: c.value == "SWD")
EXP = one(lambda c: c.value == "EXPANSION")
BAT = one(lambda c: c.value.startswith("BATT 2S"))
HOLES = find_ref(lambda c: c.value == "M3")
SW_RESET = one(lambda c: c.value == "RESET")
SW_BOOT = one(lambda c: c.value == "BOOT0")
HSE = one(lambda c: c.value == "25MHz")
LSE = one(lambda c: c.value == "32.768kHz")

for i, h in enumerate(HOLES):
    a = math.radians(45 + 90 * i)
    put(h, CX + 46.5 * math.cos(a), CY + 46.5 * math.sin(a))

put(MCU, 146.0, 96.0, 0)
put(SDRAM, 146.0, 122.5, 90)
put_edge(FPC, -90, (0, 1), (0, 0), inset=6.5)            # cable exits over the top rim
put_edge(TOUCH, -62, (0, 1), (0, 0), inset=6.0)
put_edge(USB, 0, (0, 1), (0, 3.1), inset=0.0)              # receptacle mouth flush with rim
put_edge(SD, 200, (0, -1), (0, -9.7), inset=-1.0)          # card protrudes ~1mm past rim
put(GPS, 168.0, 73.0, 0)
put(GPS_UFL, 178.5, 76.3, 0)
put(NORA, 178.0, 121.0, -90)
put(BLE_UFL, 189.6, 116.6, 0)
R_MATCH = one(lambda c: c.value == "0R")
C_SH1 = one(lambda c: c.dnp and "BLE_ANT" in c.pins.values())
C_SH2 = one(lambda c: c.dnp and "BLE_ANT_M" in c.pins.values())
put(R_MATCH, 186.0, 116.6, 0)
put(C_SH1, 184.5, 114.9, 90, check=False)
put(C_SH2, 186.6, 114.9, 90, check=False)
put(MQ, 123.0 - 3.36, 121.0 - 3.36, 0)
put(LDO33, 166.0, 142.0, 0)
put(BAT, 150.0, 146.5, 180)
put_edge(SWD, 178, (-1, 0), (0, 0), inset=6.0)
put_edge(EXP, 113, (-1, 0), (0, 8.89), inset=4.5)
put(IMU, 166.0, 84.0, 0)
put(BARO, 166.0, 92.0, 0)
put(HSE, 131.3, 98.75, 180)
C_HSE_IN = one(lambda c: c.value == "12p" and "HSE_IN" in c.pins.values())
C_HSE_OUT = one(lambda c: c.value == "12p" and "HSE_OUT" in c.pins.values())
put(C_HSE_IN, 132.4, 95.2, 90, check=False)
put(C_HSE_OUT, 130.2, 102.3, 270, check=False)
put(LSE, 131.5, 90.5, 90)
put_edge(SW_RESET, 240, (0, 1), (0, 0), inset=7.0)
put_edge(SW_BOOT, 157, (0, 1), (0, 0), inset=7.0)

# ---------------------------------------------------------------- RF pre-routes (50 ohm ~0.36 mm on L1 over L2 GND)
RF_W = 0.36

# ---------------------------------------------------------------- auto placement of the rest
POWER = {"GND", "+3V3", "+1V8", "+5V", "VBUS", "VSYS", "+BATT", "VDDA", "VCAP"}
done = {r for s in ("top",) for r in FPS if False}
manual = set(HOLES) | {MCU, SDRAM, FPC, TOUCH, USB, SD, GPS, NORA, GPS_UFL, BLE_UFL, MQ, LDO33, BAT, SWD, EXP, IMU,
                       BARO, HSE, LSE, SW_RESET, SW_BOOT, R_MATCH, C_SH1, C_SH2, C_HSE_IN, C_HSE_OUT}

def ref_nets(ref):
    return {pad_net.get((ref, p.GetNumber())) for p in FPS[ref].Pads()} - {None}

# decoupling caps on the bottom side, one per supply pin
decap_slots = {}
for c in D.COMPS:
    if c.group and c.group[0] == "decap":
        decap_slots.setdefault(c.group[1], []).append(c.ref)
for ic, caps in decap_slots.items():
    fp = FPS[ic]
    icx, icy = ToMM(fp.GetPosition().x), ToMM(fp.GetPosition().y)
    used = set()
    for cref in caps:
        net = [n for n in ref_nets(cref) if n != "GND"][0]
        pads = [p for p in fp.Pads() if pad_net.get((ic, p.GetNumber())) == net and p.GetNumber() not in used]
        if not pads:
            pads = [p for p in fp.Pads() if pad_net.get((ic, p.GetNumber())) == net]
        pad = pads[0]
        used.add(pad.GetNumber())
        px, py = ToMM(pad.GetPosition().x), ToMM(pad.GetPosition().y)
        d = math.hypot(icx - px, icy - py)
        tx, ty = px + (icx - px) / d * 3.2, py + (icy - py) / d * 3.2
        rot = 0 if abs(px - icx) > abs(py - icy) else 90
        try_place(cref, tx, ty, side="bot", rot=rot, maxr=12)
        manual.add(cref)

# everything else: near the most-connected placed part, same side as it
order = [c.ref for c in D.COMPS if not c.ref.startswith("#") and c.ref not in manual]
pending = list(order)
for _ in range(4):
    nxt = []
    for ref in pending:
        mynets = ref_nets(ref)
        sig = mynets - POWER
        best, bscore, bpt = None, 0, None
        for other in FPS:
            if other == ref or other in pending and other not in manual:
                continue
            if other not in manual:
                continue
            on = ref_nets(other)
            score = 3 * len(sig & on) + 0.5 * len((mynets & on) - {"GND"})
            if score > bscore:
                best, bscore = other, score
        if best is None:
            nxt.append(ref)
            continue
        shared = (sig & ref_nets(best)) or ((mynets & ref_nets(best)) - {"GND"})
        pts = [ToMM(p.GetPosition()) for p in FPS[best].Pads() if pad_net.get((best, p.GetNumber())) in shared]
        tx = sum(p[0] for p in pts) / len(pts)
        ty = sum(p[1] for p in pts) / len(pts)
        bx, by = ToMM(FPS[best].GetPosition().x), ToMM(FPS[best].GetPosition().y)
        dx, dy = tx - bx, ty - by
        d = math.hypot(dx, dy) or 1
        tx, ty = tx + dx / d * 2.5, ty + dy / d * 2.5
        side = "bot" if FPS[best].IsFlipped() else "top"
        try_place(ref, tx, ty, side=side)
        manual.add(ref)
    pending = nxt
    if not pending:
        break
for ref in pending:     # unconnected leftovers (test points etc.)
    try_place(ref, CX + 20, CY + 30)

# ---------------------------------------------------------------- RF pre-routes (locked)
def track(x1, y1, x2, y2, net, w, layer=pcbnew.F_Cu):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(P(x1, y1)); t.SetEnd(P(x2, y2))
    t.SetWidth(FromMM(w)); t.SetLayer(layer); t.SetNet(nets[net]); t.SetLocked(True)
    board.Add(t)

def link(a, b, net, w=RF_W):
    (x1, y1), (x2, y2) = pad_pos(*a), pad_pos(*b)
    track(x1, y1, x2, y2, net, w)

link((GPS, "11"), (GPS_UFL, "1"), "GPS_RF_IN")
link((NORA, "K9"), (R_MATCH, "1"), "BLE_ANT")
link((R_MATCH, "2"), (BLE_UFL, "1"), "BLE_ANT_M")
# 25 MHz crystal: short, via-free, HSE_OUT threads the crystal's row gap
track(135.34, 98.25, 132.8, 98.25, "HSE_IN", 0.15)
track(135.34, 98.75, 131.3, 98.75, "HSE_OUT", 0.15)
track(131.3, 98.75, 130.55, 99.5, "HSE_OUT", 0.15)
xa, ya = pad_pos(C_HSE_IN, "1"); track(xa, ya, 132.4, 97.6, "HSE_IN", 0.2)
xa, ya = pad_pos(C_HSE_OUT, "1"); track(xa, ya, 130.2, 99.9, "HSE_OUT", 0.2)
for cref, net in ((C_SH1, "BLE_ANT"), (C_SH2, "BLE_ANT_M")):
    x, y = pad_pos(cref, "1")
    track(x, y, x, 116.6, net, 0.3)

# ---------------------------------------------------------------- planes
def circle_poly(r, n=128):
    chain = pcbnew.SHAPE_LINE_CHAIN()
    for i in range(n):
        a = 2 * math.pi * i / n
        chain.Append(FromMM(CX + r * math.cos(a)), FromMM(CY + r * math.sin(a)))
    chain.SetClosed(True)
    return chain

def add_zone(netname, layer, prio=0, r=RAD - 0.5):
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(nets[netname])
    z.Outline().AddOutline(circle_poly(r))
    z.SetAssignedPriority(prio)
    z.SetLocalClearance(FromMM(0.3))
    z.SetMinThickness(FromMM(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(FromMM(0.3))
    z.SetThermalReliefSpokeWidth(FromMM(0.4))
    board.Add(z)
    return z

add_zone("GND", pcbnew.In1_Cu)
add_zone("+3V3", pcbnew.In2_Cu)

# title on silk
t = pcbnew.PCB_TEXT(board)
t.SetText("moVe test platform  rev A  4L")
t.SetPosition(P(CX, CY + 40.0))
t.SetLayer(pcbnew.B_SilkS)
t.SetMirrored(True)
t.SetTextSize(VECTOR2I(FromMM(1.2), FromMM(1.2)))
t.SetTextThickness(FromMM(0.18))
board.Add(t)

board.Save(BOARD)
print("saved", BOARD, "footprints:", len(FPS))
