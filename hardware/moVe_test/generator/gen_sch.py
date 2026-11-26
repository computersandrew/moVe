"""Generate the hierarchical KiCad 7 schematic from design.py, Adafruit-style:
functional blocks in titled dashed boxes, supply arrows up / GND down, vertical passives between rail and net,
net names on wires, same-net power pins bussed, and collision-checked text (no overlaps)."""
import math, os, re, sys, uuid, copy
from collections import defaultdict, OrderedDict
from sexp import Sym as S, dump, find, findall
import lib as L
import customlib
import design as D

PROJECT = "moVe_test"
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
os.makedirs(OUT, exist_ok=True)
NS = uuid.UUID("6b1f3d2c-0000-4000-8000-6d6f56650001")
def uid(*k):
    return str(uuid.uuid5(NS, "/".join(map(str, k))))

customlib.write_symbol_lib(os.path.join(OUT, "moVe.kicad_sym"))
customlib.write_footprint_lib(os.path.join(OUT, "moVe.pretty"))
CUSTOM = os.path.join(OUT, "moVe.kicad_sym")

def libsym(lib_id):
    if lib_id.startswith("moVe:"):
        return L.get_symbol(lib_id, CUSTOM)
    return L.get_symbol(lib_id)

G = 2.54
FONT = 1.27
CHAR = 1.05            # mm per character at 1.27 mm text (conservative)
TH = 1.5               # text box height at 1.27 mm
ROOT_UUID = uid("root")
SHEET_UUID = {s: uid("sheet", s) for s in D.SHEETS}
POWER = D.POWER_NETS
TWO_PIN = {"Device:R", "Device:C", "Device:L", "Device:FerriteBead_Small", "Device:Polyfuse", "Device:D_Schottky",
           "Device:LED", "Device:Crystal", "Switch:SW_Push"}

def snap(v, g=G):
    return round(round(v / g) * g, 4)

def rot(x, y, deg):
    a = math.radians(deg)
    c, s = round(math.cos(a)), round(math.sin(a))
    return x * c - y * s, x * s + y * c

ANGV = {0: (1, 0), 90: (0, 1), 180: (-1, 0), 270: (0, -1)}

def tw(text, size=FONT):
    return len(text) * CHAR * size / FONT

# ------------------------------------------------------------------ geometry of library symbols
def graphics_box(sym, r):
    """bbox (screen coords, symbol at origin, rotation r) of body graphics + pin lines."""
    xs, ys = [], []
    def add(x, y):
        X, Y = rot(x, y, r); xs.append(X); ys.append(-Y)
    for sub in findall(sym, "symbol"):
        parts = sub[1].rsplit("_", 2)
        if len(parts) == 3 and parts[1] not in ("0", "1"):
            continue
        for g in sub[2:]:
            if not isinstance(g, list):
                continue
            k = g[0]
            if k == "rectangle":
                s, e = find(g, "start"), find(g, "end")
                for x in (s[1], e[1]):
                    for y in (s[2], e[2]):
                        add(float(x), float(y))
            elif k == "polyline":
                for p in findall(find(g, "pts"), "xy"):
                    add(float(p[1]), float(p[2]))
            elif k == "circle":
                c, rr = find(g, "center"), float(find(g, "radius")[1])
                for dx, dy in ((rr, rr), (-rr, -rr)):
                    add(float(c[1]) + dx, float(c[2]) + dy)
            elif k == "arc":
                for key in ("start", "mid", "end"):
                    p = find(g, key)
                    add(float(p[1]), float(p[2]))
            elif k == "pin":
                at = find(g, "at"); ln = float(find(g, "length")[1])
                ax, ay = ANGV[int(float(at[3])) % 360]
                if any(x == "hide" for x in g if isinstance(x, S)):
                    continue
                add(float(at[1]), float(at[2]))
                add(float(at[1]) + ax * ln, float(at[2]) + ay * ln)
    if not xs:
        return (-1, -1, 1, 1)
    return (min(xs), min(ys), max(xs), max(ys))

def pin_geom(p, r):
    x, y = rot(p["x"], p["y"], r)
    ax, ay = ANGV[int(p["angle"]) % 360]
    ox, oy = rot(-ax, -ay, r)
    return (x, -y), (round(ox), -round(oy))

# ------------------------------------------------------------------ cells
def boxes_overlap(a, b, pad=0.0):
    return a[0] < b[2] + pad and b[0] < a[2] + pad and a[1] < b[3] + pad and b[1] < a[3] + pad

class Cell:
    """A part plus its connection decorations, drawn in local coordinates."""
    def __init__(self):
        self.prims = []        # (kind, payload)
        self.boxes = []        # (box, kind)  kind: text|body|wire
    def bbox(self):
        bs = [b for b, _ in self.boxes]
        return (min(b[0] for b in bs), min(b[1] for b in bs), max(b[2] for b in bs), max(b[3] for b in bs))
    def collides(self, boxes):
        for nb, nk in boxes:
            for b, k in self.boxes:
                if k == "wire" and nk == "wire":
                    continue
                pad = 0.3 if (k == "text" and nk == "text") else 0.12
                if boxes_overlap(nb, b, pad):
                    return True
        return False
    def wire(self, a, b):
        a = (round(a[0], 4), round(a[1], 4)); b = (round(b[0], 4), round(b[1], 4))
        if a == b:
            return
        self.prims.append(("wire", (a, b)))
        self.boxes.append(((min(a[0], b[0]) - 0.1, min(a[1], b[1]) - 0.1, max(a[0], b[0]) + 0.1, max(a[1], b[1]) + 0.1), "wire"))

PWR_COUNTER = [0]

def power_candidate(net, p, direction):
    """Power symbol at point p, body pointing `direction` (screen unit vector). Returns (prims, boxes)."""
    lid = POWER[net]
    sym = libsym(lid)
    pin = L.pins(sym)[0]
    ax, ay = ANGV[int(pin["angle"]) % 360]            # body direction in lib coords
    want = (direction[0], -direction[1])
    r = [rr for rr in (0, 90, 180, 270) if tuple(map(round, rot(ax, ay, rr))) == want][0]
    gb = graphics_box(sym, r)
    gb = (gb[0] + p[0], gb[1] + p[1], gb[2] + p[0], gb[3] + p[1])
    w = tw(net)
    dx, dy = direction
    t = 0.35
    gb = (gb[0] + (t if dx > 0 else 0), gb[1] + (t if dy > 0 else 0), gb[2] - (t if dx < 0 else 0), gb[3] - (t if dy < 0 else 0))
    if dy:   # vertical symbol, horizontal text beyond the tip
        tipy = gb[1] if dy < 0 else gb[3]
        cy = tipy + dy * (0.4 + TH / 2)
        tbox = (p[0] - w / 2, cy - TH / 2, p[0] + w / 2, cy + TH / 2)
        val = (p[0], cy, 0 if r in (0, 180) else 90)
    else:
        tipx = gb[0] if dx < 0 else gb[2]
        cx = tipx + dx * (0.5 + w / 2)
        tbox = (cx - w / 2, p[1] - TH / 2, cx + w / 2, p[1] + TH / 2)
        val = (cx, p[1], 90 if r in (90, 270) else 0)
    return [("power", (lid, net, p, r, val))], [(gb, "sym"), (tbox, "text")]

def label_candidate(net, p, direction, glob, offset=1.27):
    """net label at the end of a stub at p, extending in `direction`."""
    w = tw(net)
    dx, dy = direction
    prims, boxes = [], []
    if glob:
        L_ = w + 2.6
        ang = {(1, 0): 0, (-1, 0): 180, (0, -1): 90, (0, 1): 270}[(dx, dy)]
        if dy == 0:
            b = (min(p[0], p[0] + dx * L_), p[1] - 1.0, max(p[0], p[0] + dx * L_), p[1] + 1.0)
        else:
            b = (p[0] - 1.0, min(p[1], p[1] + dy * L_), p[0] + 1.0, max(p[1], p[1] + dy * L_))
        prims.append(("glabel", (net, p, ang)))
        boxes.append((b, "text"))
        return prims, boxes, p
    # local label: wire under the text (Adafruit style)
    L_ = math.ceil((w + offset + 1.0) / G) * G
    end = (p[0] + dx * L_, p[1] + dy * L_)
    anchor = (p[0] + dx * offset, p[1] + dy * offset)
    ang = {(1, 0): 0, (-1, 0): 180, (0, -1): 90, (0, 1): 270}[(dx, dy)]
    if dy == 0:
        b = (min(anchor[0], anchor[0] + dx * w), p[1] - 0.25 - TH, max(anchor[0], anchor[0] + dx * w), p[1] - 0.15)
    else:
        b = (p[0] - 0.25 - TH, min(anchor[1], anchor[1] + dy * w), p[0] - 0.15, max(anchor[1], anchor[1] + dy * w))
    prims.append(("wire", (p, end)))
    prims.append(("label", (net, anchor, ang)))
    boxes.append(((min(p[0], end[0]) - 0.1, min(p[1], end[1]) - 0.1, max(p[0], end[0]) + 0.1, max(p[1], end[1]) + 0.1), "wire"))
    boxes.append((b, "text"))
    return prims, boxes, end

# ------------------------------------------------------------------ net scope (local vs global labels)
def resolve(comp):
    sym = libsym(comp.lib_id)
    pins = L.pins(sym)
    assign = {}
    if "@byname" in comp.pins:
        byname = comp.pins["@byname"]
        for p in pins:
            assign[p["number"]] = byname.get(re.split(r"[(/]", p["name"])[0])
    else:
        for p in pins:
            assign[p["number"]] = comp.pins.get(p["number"])
    return sym, pins, assign

NET_SHEETS = defaultdict(set)
for c in D.COMPS:
    _, _, a = resolve(c)
    for n in a.values():
        if n:
            NET_SHEETS[n].add(c.sheet)
def is_global(net):
    return len(NET_SHEETS[net]) > 1

# ------------------------------------------------------------------ cell builders
def field_box(text, cx, cy):
    w = tw(text)
    return (cx - w / 2, cy - TH / 2, cx + w / 2, cy + TH / 2)

def unique_pins(pins, assign, r):
    seen = OrderedDict()
    for p in pins:
        (px, py), (ox, oy) = pin_geom(p, r)
        key = (round(px, 3), round(py, 3))
        net = assign.get(p["number"])
        if key in seen:
            if seen[key]["net"] is None:
                seen[key]["net"] = net
            continue
        if p["hidden"] and net is None:
            continue
        seen[key] = dict(num=p["number"], net=net, pos=(px, py), out=(ox, oy), hidden=p["hidden"], type=p["type"])
    return list(seen.values())

def place_power_at(cell, net, p, out, allow_natural=True):
    """attach power symbol at point p which is the end of a stub going `out`. Natural (up/down) if clear."""
    natural = (0, -1) if net != "GND" else (0, 1)
    opts = []
    if out == natural or (allow_natural and out[1] == 0):
        opts.append(("direct", natural))
    if out[1] != 0 and out != natural:
        opts.append(("direct", out))
    if out[1] == 0:
        opts.append(("direct", out))
    for mode, d in opts:
        prims, boxes = power_candidate(net, p, d)
        if not cell.collides(boxes):
            cell.prims += prims; cell.boxes += boxes
            return
    # last resort: extend outward and point outward
    q = (p[0] + out[0] * G, p[1] + out[1] * G)
    cell.wire(p, q)
    prims, boxes = power_candidate(net, q, out)
    cell.prims += prims; cell.boxes += boxes

def build_main(c, sheet):
    sym, pins, assign = resolve(c)
    cell = Cell()
    r = 0
    body = graphics_box(sym, r)
    cell.prims.append(("sym", dict(comp=c, x=0, y=0, r=r, pins=sorted({p["number"] for p in pins}))))
    cell.boxes.append((body, "body"))
    ups = unique_pins(pins, assign, r)
    sides = defaultdict(list)
    for u in ups:
        sides[u["out"]].append(u)
    power_runs = []
    for out, lst in sides.items():
        along = (lambda u: u["pos"][1]) if out[1] == 0 else (lambda u: u["pos"][0])
        lst.sort(key=along)
        conn = [u for u in lst if u["net"] is not None]
        # power runs of consecutive same-net power pins
        i = 0
        while i < len(conn):
            u = conn[i]
            if u["net"] in POWER:
                j = i
                while j + 1 < len(conn) and conn[j + 1]["net"] == u["net"]:
                    j += 1
                power_runs.append((out, conn[i:j + 1]))
                i = j + 1
            else:
                i += 1
    # signal pins: global flags first, then plain labels slid outward until clear
    for want_glob in (True, False):
        for u in ups:
            px, py = u["pos"]; ox, oy = u["out"]
            if u["net"] is None:
                if want_glob and not u["hidden"]:
                    cell.prims.append(("nc", (px, py)))
                continue
            if u["net"] in POWER or is_global(u["net"]) != want_glob:
                continue
            stub_end = (px + ox * G, py + oy * G)
            cell.wire((px, py), stub_end)
            for k in range(0, 30):
                prims, boxes, _ = label_candidate(u["net"], stub_end, (ox, oy), want_glob, offset=1.27 + k * G)
                if want_glob or not cell.collides([b for b in boxes if b[1] == "text"]):
                    break
            cell.prims += prims; cell.boxes += boxes
    # power runs
    for out, run in power_runs:
        ox, oy = out
        ends = []
        for u in run:
            px, py = u["pos"]
            e = (px + ox * G, py + oy * G)
            cell.wire((px, py), e)
            ends.append(e)
        for a, b in zip(ends, ends[1:]):
            cell.wire(a, b)
        for e in ends[1:-1]:
            cell.prims.append(("junction", e))
        net = run[0]["net"]
        if oy == 0:
            # vertical bus: symbol at top end for supply, bottom end for GND
            p = ends[0] if net != "GND" else ends[-1]
            if len(ends) > 1:
                cell.prims.append(("junction", p))
            place_power_at(cell, net, p, out)
        else:
            p = ends[0]
            if len(ends) > 1:
                cell.prims.append(("junction", p))
            natural = (0, -1) if net != "GND" else (0, 1)
            if out == natural:
                prims, boxes = power_candidate(net, p, natural)
                cell.prims += prims; cell.boxes += boxes
            else:
                q = (p[0] + ox * G, p[1] + oy * G)
                cell.wire(p, q)
                prims, boxes = power_candidate(net, q, out)
                cell.prims += prims; cell.boxes += boxes
    # fields: reference above everything, value below everything
    bb = cell.bbox()
    cx = (body[0] + body[2]) / 2
    ry, vy = bb[1] - 1.0 - TH / 2, bb[3] + 1.0 + TH / 2
    cell.prims[0][1]["fields"] = {"Reference": (cx, ry, 0), "Value": (cx, vy, 0)}
    cell.boxes.append((field_box(c.ref, cx, ry), "text"))
    cell.boxes.append((field_box(c.value, cx, vy), "text"))
    return cell

def build_two_pin(c, sheet):
    sym, pins, assign = resolve(c)
    p1, p2 = pins[0], pins[1]
    n1, n2 = assign[p1["number"]], assign[p2["number"]]
    P = lambda n: n in POWER
    vertical = n1 == "GND" or n2 == "GND" or (P(n1) != P(n2))
    if vertical:
        if n2 == "GND" or (P(n1) and n1 != "GND" and not P(n2)):
            top, bot = p1, p2
        else:
            top, bot = p2, p1
        r = [rr for rr in (0, 90, 180, 270) if pin_geom(top, rr)[1] == (0, -1)][0]
    else:
        r = [rr for rr in (0, 90, 180, 270) if pin_geom(p1, rr)[1] == (-1, 0)][0]
    cell = Cell()
    body = graphics_box(sym, r)
    cell.prims.append(("sym", dict(comp=c, x=0, y=0, r=r, pins=[p1["number"], p2["number"]])))
    cell.boxes.append((body, "body"))
    for p in (p1, p2):
        net = assign[p["number"]]
        (px, py), out = pin_geom(p, r)
        if net in POWER:
            if out[1] != 0:
                natural = (0, -1) if net != "GND" else (0, 1)
                if out == natural:
                    prims, boxes = power_candidate(net, (px, py), natural)
                    cell.prims += prims; cell.boxes += boxes
                else:
                    q = (px + out[0] * G, py + out[1] * G); cell.wire((px, py), q)
                    prims, boxes = power_candidate(net, q, out); cell.prims += prims; cell.boxes += boxes
            else:
                q = (px + out[0] * G, py); cell.wire((px, py), q)
                place_power_at(cell, net, q, out)
        else:
            glob = is_global(net)
            if out[1] != 0:      # vertical part: go out then turn right
                q = (px, py + out[1] * G); cell.wire((px, py), q)
                prims, boxes, _ = label_candidate(net, q, (1, 0), glob)
            else:
                q = (px + out[0] * G, py); cell.wire((px, py), q)
                prims, boxes, _ = label_candidate(net, q, out, glob)
            cell.prims += prims; cell.boxes += boxes
    # fields
    if vertical:
        fx = body[2] + 0.8
        rw, vw = tw(c.ref), tw(c.value)
        fields = {"Reference": (fx + rw / 2, -1.27, r % 180), "Value": (fx + vw / 2, 1.27, r % 180)}
        cell.boxes.append((field_box(c.ref, fx + rw / 2, -1.27), "text"))
        cell.boxes.append((field_box(c.value, fx + vw / 2, 1.27), "text"))
    else:
        ry, vy = body[1] - 0.4 - TH / 2, body[3] + 0.4 + TH / 2
        fields = {"Reference": (0, ry, r % 180), "Value": (0, vy, r % 180)}
        cell.boxes.append((field_box(c.ref, 0, ry), "text"))
        cell.boxes.append((field_box(c.value, 0, vy), "text"))
    cell.prims[0][1]["fields"] = fields
    return cell

def build_one_pin(c, sheet):
    """test points, mounting holes, PWR_FLAGs"""
    sym, pins, assign = resolve(c)
    p = pins[0]
    net = assign[p["number"]]
    cell = Cell()
    r = 0
    if c.lib_id == "Connector:TestPoint" and net in POWER and net != "GND":
        r = 180
    body = graphics_box(sym, r)
    cell.prims.append(("sym", dict(comp=c, x=0, y=0, r=r, pins=[p["number"]])))
    cell.boxes.append((body, "body"))
    (px, py), out = pin_geom(p, r)
    if net in POWER:
        natural = (0, -1) if net != "GND" else (0, 1)
        if out == natural:
            prims, boxes = power_candidate(net, (px, py), natural)
        else:   # PWR_FLAG on a supply: drop down then over, arrow up
            q1 = (px, py + G); q2 = (px + 2 * G, py + G)
            cell.wire((px, py), q1); cell.wire(q1, q2)
            prims, boxes = power_candidate(net, q2, natural)
        cell.prims += prims; cell.boxes += boxes
    else:
        q = (px, py + out[1] * 2 * G) if out[1] else (px + out[0] * G, py)
        cell.wire((px, py), q)
        prims, boxes, _ = label_candidate(net, q, (1, 0) if out[1] else out, is_global(net))
        cell.prims += prims; cell.boxes += boxes
    hide_ref = c.ref.startswith("#")
    fx = body[2] + 1.4
    fields = {}
    if not hide_ref:
        fields["Reference"] = (fx + tw(c.ref) / 2, body[1] + 0.8, 0)
        cell.boxes.append((field_box(c.ref, fx + tw(c.ref) / 2, body[1] + 0.8), "text"))
        vy = body[1] + 0.8 + TH + 0.3
    else:
        fields["Reference"] = (0, 0, 0)
        vy = body[1] + 0.8
    if c.lib_id == "power:PWR_FLAG":
        fields["Value"] = (0, body[1] - 0.4 - TH / 2, 0)
        cell.boxes.append((field_box(c.value, 0, body[1] - 0.4 - TH / 2), "text"))
    else:
        fields["Value"] = (fx + tw(c.value) / 2, vy, 0)
        cell.boxes.append((field_box(c.value, fx + tw(c.value) / 2, vy), "text"))
    cell.prims[0][1]["fields"] = fields
    cell.prims[0][1]["hide_ref"] = hide_ref
    return cell

def build_cell(c, sheet):
    sym = libsym(c.lib_id)
    n = len(L.pins(sym))
    if c.lib_id in TWO_PIN:
        return build_two_pin(c, sheet)
    if n == 1:
        return build_one_pin(c, sheet)
    return build_main(c, sheet)

# ------------------------------------------------------------------ packing
def pack(cells, max_w, gap=5.08):
    """shelf-pack cells (list of (cell, ...)); returns list of (cell, dx, dy) with grid-aligned offsets, and bbox."""
    out = []
    x, y, rowh = 0.0, 0.0, 0.0
    for cell in cells:
        b = cell.bbox()
        w, h = b[2] - b[0], b[3] - b[1]
        if x > 0 and x + w > max_w:
            x, y, rowh = 0.0, y + rowh + gap, 0.0
        dx = math.ceil((x - b[0]) / G) * G
        dy = math.ceil((y - b[1]) / G) * G
        out.append((cell, dx, dy))
        x = dx + b[2] + gap
        rowh = max(rowh, dy + b[3] - y)
    xs = [dx + c.bbox()[2] for c, dx, dy in out]
    ys = [dy + c.bbox()[3] for c, dx, dy in out]
    return out, (0, 0, max(xs), max(ys))

TITLE_SIZE = 2.54

def build_block(name, comps, sheet, max_w):
    mains = [build_cell(c, sheet) for c in comps if len(L.pins(libsym(c.lib_id))) > 2 and c.lib_id not in TWO_PIN]
    rest = [build_cell(c, sheet) for c in comps if not (len(L.pins(libsym(c.lib_id))) > 2 and c.lib_id not in TWO_PIN)]
    placed = []
    y0 = 0.0
    if mains:
        pm, bm = pack(mains, max_w, gap=7.62)
        placed += pm
        y0 = bm[3] + 7.62
    if rest:
        pr, br = pack(rest, max(max_w, 60), gap=7.62)
        for cell, dx, dy in pr:
            placed.append((cell, dx, dy + math.ceil(y0 / G) * G))
    # block extents
    bx = [(dx + c.bbox()[0], dy + c.bbox()[1], dx + c.bbox()[2], dy + c.bbox()[3]) for c, dx, dy in placed]
    x1, y1 = min(b[0] for b in bx), min(b[1] for b in bx)
    x2, y2 = max(b[2] for b in bx), max(b[3] for b in bx)
    tw_ = tw(name, TITLE_SIZE) * 1.15
    x2 = max(x2, x1 + tw_ + 2)
    m = 3.81
    rect = (x1 - m, y1 - m, x2 + m, y2 + m + TITLE_SIZE + 2.0)
    title = (rect[0] + 2.0, rect[3] - 1.5)
    return dict(name=name, placed=placed, rect=rect, title=title)

# ------------------------------------------------------------------ emission
all_text_boxes = []

def prop_entry(key, val, x, y, ang=0, hide=False):
    eff = [S("effects"), [S("font"), [S("size"), FONT, FONT]]]
    if hide:
        eff.append(S("hide"))
    return [S("property"), key, val, [S("at"), round(x, 3), round(y, 3), ang], eff]

def emit_symbol(lib_ids, items, sheet, lib_id, ref, value, footprint, x, y, r, pins, fields, extra=(), dnp=False,
                hide_ref=False, in_bom=True, on_board=True, key=None):
    lib_ids.add(lib_id)
    k = key or ref
    path = f"/{ROOT_UUID}/{SHEET_UUID[sheet]}"
    e = [S("symbol"), [S("lib_id"), lib_id], [S("at"), round(x, 4), round(y, 4), r], [S("unit"), 1],
         [S("in_bom"), S("yes" if in_bom else "no")], [S("on_board"), S("yes" if on_board else "no")],
         [S("dnp"), S("yes" if dnp else "no")], [S("uuid"), uid("sym", sheet, k)]]
    rx, ry, ra = fields["Reference"]
    vx, vy, va = fields["Value"]
    e.append(prop_entry("Reference", ref, x + rx, y + ry, ra, hide=hide_ref))
    e.append(prop_entry("Value", value, x + vx, y + vy, va))
    e.append(prop_entry("Footprint", footprint, x, y, 0, hide=True))
    e.append(prop_entry("Datasheet", "", x, y, 0, hide=True))
    for kk, vv in extra:
        e.append(prop_entry(kk, vv, x, y, 0, hide=True))
    for pn in pins:
        e.append([S("pin"), pn, [S("uuid"), uid("pin", sheet, k, pn)]])
    e.append([S("instances"), [S("project"), PROJECT, [S("path"), path, [S("reference"), ref], [S("unit"), 1]]]])
    items.append(e)

def build_sheet(sheet):
    comps = [c for c in D.COMPS if c.sheet == sheet]
    blocks = OrderedDict()
    for c in comps:
        blocks.setdefault(c.block, []).append(c)
    for paper, W, H in (("A4", 297, 210), ("A3", 420, 297), ("A2", 594, 420), ("A1", 841, 594)):
        max_w = W - 40
        built = [build_block(n, cs, sheet, min(max_w, 250)) for n, cs in blocks.items()]
        # shelf-pack blocks into rows
        rows, cur, x = [], [], 0.0
        for b in built:
            r = b["rect"]; w = r[2] - r[0]
            if cur and x + w > max_w:
                rows.append(cur); cur, x = [], 0.0
            cur.append(b); x += w + 5.08
        if cur:
            rows.append(cur)
        heights = [max(b["rect"][3] - b["rect"][1] for b in row) for row in rows]
        total_h = sum(heights) + 5.08 * (len(rows) - 1)
        widest = max(sum(b["rect"][2] - b["rect"][0] for b in row) + 5.08 * (len(row) - 1) for row in rows)
        if total_h <= H - 75 and widest <= max_w:
            break
    # justify: rows fill the page width, row heights grow to fill the page height
    avail_h = H - 75
    grow = max(0.0, avail_h - total_h)
    pos = []
    y = 0.0
    for row, hgt in zip(rows, heights):
        hgt2 = min(hgt + grow * hgt / sum(heights), hgt * 1.35 + 10)
        nat = sum(b["rect"][2] - b["rect"][0] for b in row)
        extra = max(0.0, max_w - nat - 5.08 * (len(row) - 1))
        x = 0.0
        for b in row:
            r = b["rect"]; w = r[2] - r[0]; h = r[3] - r[1]
            w2 = w + extra * w / nat
            # content offset: center inside the enlarged box (keep title at bottom-left)
            cx = math.floor((w2 - w) / 2 / G) * G
            cy = math.floor((hgt2 - h) / 2 / G) * G
            b["rect2"] = (r[0] - cx, r[1] - cy, r[0] - cx + w2, r[1] - cy + hgt2)
            pos.append((b, x - r[0] + cx, y - r[1] + cy))
            x += w2 + 5.08
        y += hgt2 + 5.08
    items, lib_ids, texts = [], set(), []
    ox0, oy0 = 20.32, 22.86
    for bi, (b, bx, by) in enumerate(pos):
        BX, BY = snap(ox0 + bx), snap(oy0 + by)
        r = b["rect2"]
        items.append([S("rectangle"), [S("start"), round(BX + r[0], 3), round(BY + r[1], 3)],
                      [S("end"), round(BX + r[2], 3), round(BY + r[3], 3)],
                      [S("stroke"), [S("width"), 0.254], [S("type"), S("dash")], [S("color"), 0, 32, 128, 1]],
                      [S("fill"), [S("type"), S("none")]], [S("uuid"), uid("blk", sheet, b["name"])]])
        tx, ty = BX + r[0] + 2.0, BY + r[3] - 1.5
        items.append([S("text"), b["name"], [S("at"), round(tx, 3), round(ty, 3), 0],
                      [S("effects"), [S("font"), [S("size"), TITLE_SIZE, TITLE_SIZE], [S("thickness"), 0.508], S("bold"), [S("color"), 0, 32, 128, 1]],
                       [S("justify"), S("left"), S("bottom")]], [S("uuid"), uid("blkt", sheet, b["name"])]])
        texts.append(((tx, ty - TITLE_SIZE * 1.2, tx + tw(b["name"], TITLE_SIZE) * 1.15, ty), f"title:{b['name']}"))
        for ci, (cell, dx, dy) in enumerate(b["placed"]):
            X, Y = BX + dx, BY + dy
            for bx_, kind in cell.boxes:
                if kind == "text":
                    texts.append(((bx_[0] + X, bx_[1] + Y, bx_[2] + X, bx_[3] + Y), f"{sheet}:{bi}:{ci}"))
            for kind, d in cell.prims:
                if kind == "sym":
                    c = d["comp"]
                    props = [("MPN", c.mpn)] if c.mpn else []
                    emit_symbol(lib_ids, items, sheet, c.lib_id, c.ref, c.value, c.footprint, X + d["x"], Y + d["y"], d["r"],
                                d["pins"], d["fields"], props, c.dnp, hide_ref=d.get("hide_ref", False),
                                in_bom=not c.ref.startswith("#"), on_board=not c.ref.startswith("#"))
                elif kind == "wire":
                    (a, bb) = d
                    items.append([S("wire"), [S("pts"), [S("xy"), round(a[0] + X, 4), round(a[1] + Y, 4)],
                                              [S("xy"), round(bb[0] + X, 4), round(bb[1] + Y, 4)]],
                                  [S("stroke"), [S("width"), 0], [S("type"), S("default")]],
                                  [S("uuid"), uid("w", sheet, bi, ci, a, bb)]])
                elif kind == "junction":
                    items.append([S("junction"), [S("at"), round(d[0] + X, 4), round(d[1] + Y, 4)], [S("diameter"), 0],
                                  [S("color"), 0, 0, 0, 0], [S("uuid"), uid("j", sheet, bi, ci, d)]])
                elif kind == "nc":
                    items.append([S("no_connect"), [S("at"), round(d[0] + X, 4), round(d[1] + Y, 4)],
                                  [S("uuid"), uid("nc", sheet, bi, ci, d)]])
                elif kind == "label":
                    net, p, ang = d
                    just = [S("left"), S("bottom")] if ang in (0, 90) else [S("right"), S("bottom")]
                    items.append([S("label"), net, [S("at"), round(p[0] + X, 4), round(p[1] + Y, 4), ang],
                                  [S("fields_autoplaced")],
                                  [S("effects"), [S("font"), [S("size"), FONT, FONT]], [S("justify")] + just],
                                  [S("uuid"), uid("l", sheet, bi, ci, net, p)]])
                elif kind == "glabel":
                    net, p, ang = d
                    just = "left" if ang in (0, 90) else "right"
                    items.append([S("global_label"), net, [S("shape"), S("passive")],
                                  [S("at"), round(p[0] + X, 4), round(p[1] + Y, 4), ang], [S("fields_autoplaced")],
                                  [S("effects"), [S("font"), [S("size"), FONT, FONT]], [S("justify"), S(just)]],
                                  [S("uuid"), uid("gl", sheet, bi, ci, net, p)],
                                  [S("property"), "Intersheetrefs", "${INTERSHEET_REFS}",
                                   [S("at"), round(p[0] + X, 4), round(p[1] + Y, 4), 0],
                                   [S("effects"), [S("font"), [S("size"), FONT, FONT]], S("hide")]]])
                elif kind == "power":
                    lid, net, p, r_, val = d
                    PWR_COUNTER[0] += 1
                    ref = f"#PWR{PWR_COUNTER[0]:04d}"
                    emit_symbol(lib_ids, items, sheet, lid, ref, net, "", p[0] + X, p[1] + Y, r_, ["1"],
                                {"Reference": (0, 0, 0), "Value": (val[0] - p[0], val[1] - p[1], val[2])},
                                hide_ref=True, in_bom=False, on_board=False)
    return paper, items, lib_ids, texts

def lib_symbols_block(lib_ids):
    out = [S("lib_symbols")]
    for lid in sorted(lib_ids):
        s = copy.deepcopy(libsym(lid)); s[1] = lid; out.append(s)
    return out

def title_block(title):
    return [S("title_block"), [S("title"), title], [S("date"), "2026-11-26"], [S("rev"), "A"],
            [S("company"), "moVe - RPI"],
            [S("comment"), 1, "4.2 in (106.68 mm) round test platform, 4 layers"]]

# ------------------------------------------------------------------ text overlap check
def check_overlaps(texts):
    bad = []
    for i in range(len(texts)):
        for j in range(i + 1, len(texts)):
            if boxes_overlap(texts[i][0], texts[j][0], -0.05):
                bad.append((texts[i], texts[j]))
    return bad

pages = {}
overlaps_total = 0
for si, sheet in enumerate(D.SHEETS):
    paper, items, lib_ids, texts = build_sheet(sheet)
    ov = check_overlaps(texts)
    overlaps_total += len(ov)
    for a, b in ov[:5]:
        print("  OVERLAP", a, b)
    tree = [S("kicad_sch"), [S("version"), 20230121], [S("generator"), S("eeschema")],
            [S("uuid"), SHEET_UUID[sheet]], [S("paper"), paper],
            title_block("moVe test platform - " + D.SHEET_TITLES[sheet]),
            lib_symbols_block(lib_ids)] + items
    fn = sheet.lower() + ".kicad_sch"
    open(os.path.join(OUT, fn), "w").write(dump(tree) + "\n")
    pages[sheet] = (fn, si + 2, paper)
    print(f"{sheet}: {sum(1 for c in D.COMPS if c.sheet == sheet)} parts on {paper}, {len(texts)} text items, {len(ov)} overlaps")
print("TOTAL TEXT OVERLAPS:", overlaps_total)

# ------------------------------------------------------------------ root
root_items = []
for i, sheet in enumerate(D.SHEETS):
    col, row = i % 4, i // 4
    x, y = 25.4 + col * 66.04, 45.72 + row * 45.72
    fn, page, _ = pages[sheet]
    root_items.append([S("sheet"), [S("at"), x, y], [S("size"), 50.8, 25.4], [S("fields_autoplaced")],
                       [S("stroke"), [S("width"), 0.1524], [S("type"), S("solid")]],
                       [S("fill"), [S("color"), 0, 0, 0, 0.0]], [S("uuid"), SHEET_UUID[sheet]],
                       [S("property"), "Sheetname", sheet, [S("at"), x, y - 0.7, 0],
                        [S("effects"), [S("font"), [S("size"), 1.27, 1.27]], [S("justify"), S("left"), S("bottom")]]],
                       [S("property"), "Sheetfile", fn, [S("at"), x, y + 26.0, 0],
                        [S("effects"), [S("font"), [S("size"), 1.27, 1.27]], [S("justify"), S("left"), S("top")]]],
                       [S("instances"), [S("project"), PROJECT, [S("path"), f"/{ROOT_UUID}", [S("page"), str(page)]]]]])
notes = ("moVe TEST PLATFORM - 4.2 in (106.68 mm) round PCB\n"
         "Stackup (4 layers, 1.6 mm): L1 signal | L2 solid GND | L3 +3V3 plane | L4 signal\n"
         "Sheet-local nets use plain labels; nets shared between sheets use global labels.\n"
         "Firmware: PWR_LDO_SUPPLY, HSE 25 MHz, LSE 32.768 kHz, ICM-20948 at 0x69. See README_moVe_test.md.")
root_items.append([S("text"), notes, [S("at"), 25.4, 152.4, 0],
                   [S("effects"), [S("font"), [S("size"), 2, 2]], [S("justify"), S("left"), S("bottom")]],
                   [S("uuid"), uid("notes")]])
root = [S("kicad_sch"), [S("version"), 20230121], [S("generator"), S("eeschema")], [S("uuid"), ROOT_UUID],
        [S("paper"), "A4"], title_block("moVe test platform - top level"), [S("lib_symbols")]] + root_items + \
       [[S("sheet_instances"), [S("path"), "/", [S("page"), "1"]]]]
open(os.path.join(OUT, PROJECT + ".kicad_sch"), "w").write(dump(root) + "\n")
open(os.path.join(OUT, "sym-lib-table"), "w").write(
    '(sym_lib_table\n  (lib (name "moVe")(type "KiCad")(uri "${KIPRJMOD}/moVe.kicad_sym")(options "")(descr "moVe project symbols"))\n)\n')
open(os.path.join(OUT, "fp-lib-table"), "w").write(
    '(fp_lib_table\n  (lib (name "moVe")(type "KiCad")(uri "${KIPRJMOD}/moVe.pretty")(options "")(descr "moVe project footprints"))\n)\n')
print("done")
