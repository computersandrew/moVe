"""Silkscreen cleanup: (1) footprint silk graphics that overlap pads or another part's silk move to the Fab layer,
(2) reference designators are re-placed where they clear pads, other silk and other references (hidden if no spot)."""
import math, sys
import pcbnew
from pcbnew import FromMM, ToMM, VECTOR2I

PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
CX, CY, RAD = 150.0, 100.0, 4.2 * 25.4 / 2
MASK = 0.06       # solder-mask expansion margin assumed around pads
GAP = 0.12        # min silk-to-silk / silk-to-mask gap we aim for

def box_of(bb, m=0.0):
    return (ToMM(bb.GetLeft()) - m, ToMM(bb.GetTop()) - m, ToMM(bb.GetRight()) + m, ToMM(bb.GetBottom()) + m)

def boxes_hit(a, c):
    return a[0] < c[2] and c[0] < a[2] and a[1] < c[3] and c[1] < a[3]

def d_pt_box(x, y, bx):
    return math.hypot(max(bx[0] - x, 0, x - bx[2]), max(bx[1] - y, 0, y - bx[3]))

def seg_box_dist(a, c, bx, n=None):
    n = n or max(2, int(math.dist(a, c) / 0.05))
    return min(d_pt_box(a[0] + (c[0] - a[0]) * k / n, a[1] + (c[1] - a[1]) * k / n, bx) for k in range(n + 1))

def seg_seg_dist(a, c, p, q):
    n = max(2, int(math.dist(a, c) / 0.05))
    best = 1e9
    vx, vy = q[0] - p[0], q[1] - p[1]
    L2 = vx * vx + vy * vy
    for k in range(n + 1):
        x, y = a[0] + (c[0] - a[0]) * k / n, a[1] + (c[1] - a[1]) * k / n
        t = 0 if L2 == 0 else max(0, min(1, ((x - p[0]) * vx + (y - p[1]) * vy) / L2))
        best = min(best, math.hypot(x - p[0] - t * vx, y - p[1] - t * vy))
    return best

SIDE = {pcbnew.F_SilkS: pcbnew.F_Cu, pcbnew.B_SilkS: pcbnew.B_Cu}

# pads by copper side (mask openings)
pads = {pcbnew.F_Cu: [], pcbnew.B_Cu: []}
for p in b.GetPads():
    for cu in pads:
        if p.IsOnLayer(cu):
            pads[cu].append((p.GetParent().GetReference(), box_of(p.GetBoundingBox(), MASK)))

# silk graphics per footprint
graphics = []    # (ref, layer, item, kind, geom, width)
for fp in b.GetFootprints():
    for it in fp.GraphicalItems():
        if it.GetClass() != "MGRAPHIC" or it.GetLayer() not in SIDE:
            continue
        w = ToMM(it.GetWidth())
        if it.GetShape() == pcbnew.SHAPE_T_SEGMENT:
            geom = ("seg", ToMM(it.GetStart()), ToMM(it.GetEnd()))
        else:
            geom = ("box", box_of(it.GetBoundingBox()))
        graphics.append((fp.GetReference(), it.GetLayer(), it, geom, w))

def g_clear_box(geom, w, bx, gap):
    if geom[0] == "seg":
        return seg_box_dist(geom[1], geom[2], bx) >= w / 2 + gap
    return not boxes_hit(geom[1], (bx[0] - w / 2 - gap, bx[1] - w / 2 - gap, bx[2] + w / 2 + gap, bx[3] + w / 2 + gap))

def g_clear_g(g1, w1, g2, w2, gap):
    if g1[0] == "seg" and g2[0] == "seg":
        return seg_seg_dist(g1[1], g1[2], g2[1], g2[2]) >= (w1 + w2) / 2 + gap
    if g2[0] == "box":
        return g_clear_box(g1, w1 + w2, g2[1], gap)
    return g_clear_box(g2, w1 + w2, g1[1], gap)

# (1) graphics: drop to Fab when they hit any pad mask or another footprint's silk
moved = 0
alive = []
for ref, layer, it, geom, w in graphics:
    bad = any(not g_clear_box(geom, w, bx, 0.0) for _, bx in pads[SIDE[layer]])
    if not bad:
        for ref2, layer2, it2, geom2, w2 in alive:
            if layer2 == layer and ref2 != ref and not g_clear_g(geom, w, geom2, w2, 0.0):
                bad = True; break
    if bad:
        it.SetLayer(pcbnew.F_Fab if layer == pcbnew.F_SilkS else pcbnew.B_Fab)
        moved += 1
    else:
        alive.append((ref, layer, it, geom, w))
print("silk graphics moved to Fab:", moved)

# board-level silk text (title) as obstacle
texts = []
for d in b.GetDrawings():
    if d.GetClass() == "PTEXT" and d.GetLayer() in SIDE:
        texts.append((d.GetLayer(), box_of(d.GetBoundingBox(), 0.0)))

def text_box(t):
    return box_of(t.GetBoundingBox())

# (2) references
placed, hidden, kept = 0, 0, 0
fps = sorted(b.GetFootprints(), key=lambda f: -ToMM(f.GetBoundingBox(False, False).GetWidth()))
for fp in fps:
    t = fp.Reference()
    if not t.IsVisible():
        continue
    layer = t.GetLayer()
    if layer not in SIDE:
        continue
    cb = box_of(fp.GetBoundingBox(False, False))
    cx, cy = (cb[0] + cb[2]) / 2, (cb[1] + cb[3]) / 2
    hw, hh = (cb[2] - cb[0]) / 2, (cb[3] - cb[1]) / 2
    def ok(tb):
        if any(math.hypot(x - CX, y - CY) > RAD - 0.4 for x in (tb[0], tb[2]) for y in (tb[1], tb[3])):
            return False
        g = (tb[0] - GAP, tb[1] - GAP, tb[2] + GAP, tb[3] + GAP)
        for _, bx in pads[SIDE[layer]]:
            if boxes_hit(g, bx):
                return False
        for _, l2, _, geom, w in alive:
            if l2 == layer and not g_clear_box(geom, w, tb, GAP):
                return False
        for l2, bx in texts:
            if l2 == layer and boxes_hit(g, bx):
                return False
        return True
    found = False
    for size in (0.8, 0.7, 0.6):
        t.SetTextSize(VECTOR2I(FromMM(size), FromMM(size)))
        t.SetTextThickness(FromMM(0.12 if size > 0.65 else 0.1))
        cands = []
        for k in range(0, 14):
            o = 0.25 * k
            cands += [(cx, cb[1] - 0.5 - o, 0), (cx, cb[3] + 0.5 + o, 0),
                      (cb[0] - 0.5 - o, cy, 90), (cb[2] + 0.5 + o, cy, 90),
                      (cb[0] - 0.5 - o, cb[1] - 0.4 - o, 0), (cb[2] + 0.5 + o, cb[1] - 0.4 - o, 0),
                      (cb[0] - 0.5 - o, cb[3] + 0.4 + o, 0), (cb[2] + 0.5 + o, cb[3] + 0.4 + o, 0),
                      (cb[0] - 0.8 - o, cy, 0), (cb[2] + 0.8 + o, cy, 0), (cx, cb[1] - 0.8 - o, 90), (cx, cb[3] + 0.8 + o, 90)]
        for x, y, ang in cands:
            t.SetTextAngleDegrees(ang)
            t.SetPosition(VECTOR2I(FromMM(x), FromMM(y)))
            tb = text_box(t)
            if ok(tb):
                texts.append((layer, tb)); found = True; break
        if found:
            break
    if found:
        placed += 1
    else:
        t.SetVisible(False); hidden += 1
print(f"references placed: {placed}, hidden (no clear spot): {hidden}")
b.Save(PCB)
