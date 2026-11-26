"""Add plane fanout (short stub + locked via) for every SMD pad on GND / +3V3, before autorouting.
Also removes outer-layer GND pours (re-added after routing)."""
import math, sys
import pcbnew
from pcbnew import FromMM, ToMM, VECTOR2I

PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
CX, CY, RAD = 150.0, 100.0, 4.2 * 25.4 / 2
PLANE = {"GND", "+3V3"}
VIA_D, VIA_DR, CLR, STUB_W = 0.6, 0.3, 0.2, 0.25
INWARD = {"U1", "U5"}  # fan big ICs out under their bodies

# obstacles: (layerset, kind, geom, net)
obs = []
for p in b.GetPads():
    bb = p.GetBoundingBox()
    box = (ToMM(bb.GetLeft()), ToMM(bb.GetTop()), ToMM(bb.GetRight()), ToMM(bb.GetBottom()))
    layers = set()
    if p.IsOnLayer(pcbnew.F_Cu): layers.add("F")
    if p.IsOnLayer(pcbnew.B_Cu): layers.add("B")
    hole = p.GetDrillSize().x > 0
    obs.append(dict(L=layers, box=box, net=p.GetNetname(), hole=hole, pad=p))
for t in b.GetTracks():
    if t.GetClass() == "PCB_TRACK":
        s, e = ToMM(t.GetStart()), ToMM(t.GetEnd())
        L = {"F"} if t.GetLayer() == pcbnew.F_Cu else {"B"}
        obs.append(dict(L=L, seg=(s, e, ToMM(t.GetWidth()) / 2), net=t.GetNetname(), hole=False))

def d_pt_box(x, y, box):
    dx = max(box[0] - x, 0, x - box[2]); dy = max(box[1] - y, 0, y - box[3])
    return math.hypot(dx, dy)

def d_pt_seg(x, y, a, c):
    ax, ay = a; cx, cy = c
    vx, vy = cx - ax, cy - ay
    L2 = vx * vx + vy * vy
    t = 0 if L2 == 0 else max(0, min(1, ((x - ax) * vx + (y - ay) * vy) / L2))
    return math.hypot(x - ax - t * vx, y - ay - t * vy)

def d_seg_box(a, c, box, n=12):
    return min(d_pt_box(a[0] + (c[0] - a[0]) * i / n, a[1] + (c[1] - a[1]) * i / n, box) for i in range(n + 1))

def d_seg_seg(a, c, p, q, n=12):
    return min(d_pt_seg(a[0] + (c[0] - a[0]) * i / n, a[1] + (c[1] - a[1]) * i / n, p, q) for i in range(n + 1))

def via_ok(x, y, net):
    if math.hypot(x - CX, y - CY) > RAD - 0.3 - VIA_D / 2:
        return False
    r = VIA_D / 2
    for o in obs:
        if "box" in o:
            if o["box"][2] < x - 3 or o["box"][0] > x + 3 or o["box"][3] < y - 3 or o["box"][1] > y + 3:
                continue
            d = d_pt_box(x, y, o["box"])
            if o["net"] == net and not o["hole"]:
                continue
            need = r + (0.25 if o["hole"] else CLR)
            if o["net"] == net and o["hole"]:
                need = r + 0.25
            if d < need:
                return False
        else:
            a, c, w = o["seg"]
            if min(a[0], c[0]) > x + 3 or max(a[0], c[0]) < x - 3 or min(a[1], c[1]) > y + 3 or max(a[1], c[1]) < y - 3:
                continue
            if o["net"] == net and not o.get("via"):
                continue
            need = r + w + (0.25 if o.get("via") else CLR)
            if d_pt_seg(x, y, a, c) < need:
                return False
    return True

def stub_ok(a, c, layer, net, own_pad):
    global STUB_W
    for o in obs:
        if layer not in o["L"] or o["net"] == net:
            continue
        if "box" in o:
            if d_seg_box(a, c, o["box"]) < STUB_W / 2 + 0.15:
                return False
        else:
            p, q, w = o["seg"]
            if d_seg_seg(a, c, p, q) < STUB_W / 2 + w + 0.15:
                return False
    return True

added = 0
failed = []
for fp in b.GetFootprints():
    fx, fy = ToMM(fp.GetPosition())
    for p in fp.Pads():
        net = p.GetNetname()
        if net not in PLANE or p.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
            continue
        layer = "B" if p.IsOnLayer(pcbnew.B_Cu) else "F"
        px, py = ToMM(p.GetPosition())
        bb = p.GetBoundingBox()
        half = max(ToMM(bb.GetWidth()), ToMM(bb.GetHeight())) / 2
        STUB_W = min(0.25, min(ToMM(bb.GetWidth()), ToMM(bb.GetHeight())))
        base = math.atan2(py - fy, px - fx) if (px, py) != (fx, fy) else 0.0
        bases = [base + math.pi, base] if fp.GetReference() in INWARD else [base]
        done = False
        for base, dist in [(bb_, d_) for bb_ in bases for d_ in [half + 0.55 + 0.15 * k for k in range(30)]]:
            cands = [base + math.radians(da) for da in (0, 15, -15, 30, -30, 45, -45, 60, -60, 90, -90, 120, -120, 150, -150, 180)]
            cands += [math.radians(fp.GetOrientationDegrees() + k) for k in (90, -90, 0, 180)]
            for a in cands:
                vx, vy = px + dist * math.cos(a), py + dist * math.sin(a)
                vx, vy = round(vx, 3), round(vy, 3)
                if not via_ok(vx, vy, net):
                    continue
                if not stub_ok((px, py), (vx, vy), layer, net, p):
                    continue
                v = pcbnew.PCB_VIA(b)
                v.SetPosition(VECTOR2I(FromMM(vx), FromMM(vy)))
                v.SetWidth(FromMM(VIA_D)); v.SetDrill(FromMM(VIA_DR))
                v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
                v.SetNet(p.GetNet()); v.SetLocked(True)
                b.Add(v)
                t = pcbnew.PCB_TRACK(b)
                t.SetStart(p.GetPosition()); t.SetEnd(VECTOR2I(FromMM(vx), FromMM(vy)))
                t.SetWidth(FromMM(STUB_W)); t.SetLayer(pcbnew.B_Cu if layer == "B" else pcbnew.F_Cu)
                t.SetNet(p.GetNet()); t.SetLocked(True)
                b.Add(t)
                obs.append(dict(L={"F", "B"}, seg=((vx, vy), (vx, vy), VIA_D / 2), net=net, hole=True, via=True))
                obs.append(dict(L={layer}, seg=((px, py), (vx, vy), STUB_W / 2), net=net, hole=False))
                added += 1
                done = True
                break
            if done:
                break
        if not done:
            failed.append(f"{fp.GetReference()}.{p.GetNumber()}")

b.Save(PCB)
print(f"fanout vias: {added}, failed: {len(failed)} {failed[:30]}")
