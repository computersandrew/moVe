"""Rip-up-and-reroute grid router for the moVe board (2 outer layers + through vias).
usage: ripup_route.py board.kicad_pcb NET [NET ...]   (listed nets are ripped and rerouted first)
Unlocked routing of other nets may be ripped (with a cost) and is then rerouted the same way."""
import heapq, math, os, sys, time
import numpy as np
import pcbnew
from pcbnew import FromMM, ToMM, VECTOR2I

PCB = sys.argv[1]
QUEUE = list(sys.argv[2:])
b = pcbnew.LoadBoard(PCB)
G = 0.05
X0, Y0 = 96.0, 46.0
N = int(108 / G)
CX, CY, RAD = 150.0, 100.0, 4.2 * 25.4 / 2
VIA_D, VIA_DR = 0.6, 0.3
LAY = {pcbnew.F_Cu: 0, pcbnew.B_Cu: 1}
LAYID = {0: pcbnew.F_Cu, 1: pcbnew.B_Cu}
CLASS_W = {"VSYS": 0.5, "+5V": 0.5, "+BATT": 0.5, "VBUS": 0.3, "BL_SW": 0.3, "MQ7_HEAT_N": 0.3,
           "GPS_RF_IN": 0.36, "BLE_ANT": 0.36, "BLE_ANT_M": 0.36, "USB_DP": 0.25, "USB_DM": 0.25,
           "USB_C_DP": 0.25, "USB_C_DM": 0.25}
CLASS_C = {"VSYS": 0.2, "+5V": 0.2, "+BATT": 0.2, "GPS_RF_IN": 0.3, "BLE_ANT": 0.3, "BLE_ANT_M": 0.3}
MARGIN = 0.008
VIA_COST = {"HSE_OUT": 4000, "HSE_IN": 4000, "OSC32_IN": 4000, "OSC32_OUT": 4000}
SOFT_PEN = 30.0
ys, xs = Y0 + np.arange(N) * G, X0 + np.arange(N) * G
XX, YY = np.meshgrid(xs, ys, indexing="xy")

def clr(a, c):
    return max(0.127, CLASS_C.get(a, 0), CLASS_C.get(c, 0)) + MARGIN

def idx(x, y):
    return int(round((y - Y0) / G)), int(round((x - X0) / G))

# ---------------------------------------------------------------- item model
items = []
def mk_pad(p):
    bb = p.GetBoundingBox()
    return dict(kind="pad", L={LAY[l] for l in LAY if p.IsOnLayer(l)},
                box=(ToMM(bb.GetLeft()), ToMM(bb.GetTop()), ToMM(bb.GetRight()), ToMM(bb.GetBottom())),
                net=p.GetNetname(), hole=p.GetDrillSize().x > 0, locked=True, obj=None)
def mk_track(t):
    u = t.m_Uuid.AsString()
    if t.GetClass() == "PCB_VIA":
        x, y = ToMM(t.GetPosition())
        return dict(kind="via", L={0, 1}, seg=((x, y), (x, y), ToMM(t.GetWidth()) / 2), net=t.GetNetname(),
                    hole=True, locked=t.IsLocked(), obj=u)
    return dict(kind="trk", L={LAY[t.GetLayer()]}, seg=(ToMM(t.GetStart()), ToMM(t.GetEnd()), ToMM(t.GetWidth()) / 2),
                net=t.GetNetname(), hole=False, locked=t.IsLocked(), obj=u)
for p in b.GetPads():
    items.append(mk_pad(p))
for fp in b.GetFootprints():
    for z in fp.Zones():
        if z.GetIsRuleArea() and z.GetDoNotAllowTracks():
            bb = z.Outline().BBox()
            items.append(dict(kind="keepout", L={LAY[l] for l in LAY if z.GetLayerSet().Contains(l)},
                              box=(ToMM(bb.GetLeft()), ToMM(bb.GetTop()), ToMM(bb.GetRight()), ToMM(bb.GetBottom())),
                              net="__keepout__", hole=True, locked=True, obj=None))
for t in b.GetTracks():
    items.append(mk_track(t))

def dist_field(it, sx, sy):
    if "box" in it:
        x1, y1, x2, y2 = it["box"]
        return np.hypot(np.maximum(np.maximum(x1 - sx, 0), sx - x2), np.maximum(np.maximum(y1 - sy, 0), sy - y2))
    (ax, ay), (cx, cy), hw = it["seg"]
    vx, vy = cx - ax, cy - ay
    L2 = vx * vx + vy * vy
    t = np.zeros_like(sx) if L2 == 0 else np.clip(((sx - ax) * vx + (sy - ay) * vy) / L2, 0, 1)
    return np.hypot(sx - ax - t * vx, sy - ay - t * vy) - hw

def window(it, m):
    if "box" in it:
        x1, y1, x2, y2 = it["box"]
    else:
        (ax, ay), (cx, cy), hw = it["seg"]
        x1, y1, x2, y2 = min(ax, cx) - hw, min(ay, cy) - hw, max(ax, cx) + hw, max(ay, cy) + hw
    r1, c1 = idx(x1 - m, y1 - m); r2, c2 = idx(x2 + m, y2 + m)
    return max(r1, 0), max(c1, 0), min(r2, N - 1), min(c2, N - 1)

def raster(net, hw):
    hard = np.zeros((2, N, N), bool)
    soft = np.zeros((2, N, N), np.int16)
    vhard = np.zeros((N, N), bool)
    vsoft = np.zeros((N, N), np.int16)
    rr = np.hypot(XX - CX, YY - CY)
    hard[:] |= (rr > RAD - 0.3 - hw)[None]
    vhard |= rr > RAD - 0.3 - VIA_D / 2
    for it in items:
        same = it["net"] == net
        if same and not it["hole"]:
            continue
        c = clr(net, it["net"])
        r1, c1, r2, c2 = window(it, max(hw, VIA_D / 2) + c + 0.1)
        if r2 < r1 or c2 < c1:
            continue
        sx, sy = XX[r1:r2 + 1, c1:c2 + 1], YY[r1:r2 + 1, c1:c2 + 1]
        d = dist_field(it, sx, sy)
        vneed = VIA_D / 2 + (0.25 + MARGIN if it["hole"] else c)
        if same:     # same-net holes: only hole-to-hole spacing for new vias
            if it["kind"] == "pad":
                vhard[r1:r2 + 1, c1:c2 + 1] |= d < 0.0   # vias on own THT pad fine
            else:
                vhard[r1:r2 + 1, c1:c2 + 1] |= (d + it["seg"][2]) < VIA_D / 2 + VIA_D / 2 + 0.25
            continue
        tb = d < hw + c
        vb = d < vneed
        if it["locked"]:
            for l in it["L"]:
                hard[l, r1:r2 + 1, c1:c2 + 1] |= tb
            vhard[r1:r2 + 1, c1:c2 + 1] |= vb if it["L"] else False
        else:
            for l in it["L"]:
                soft[l, r1:r2 + 1, c1:c2 + 1] += tb
            vsoft[r1:r2 + 1, c1:c2 + 1] += vb
    return hard, soft, vhard, vsoft

# ---------------------------------------------------------------- connectivity of a net
def touching(a, c):
    if not (a["L"] & c["L"]):
        return False
    if "box" in a and "box" in c:
        A, B = a["box"], c["box"]
        return A[0] <= B[2] and B[0] <= A[2] and A[1] <= B[3] and B[1] <= A[3]
    if "box" in c:
        a, c = c, a
    if "box" in a:
        (p, q, hw) = c["seg"]
        n = max(2, int(math.dist(p, q) / 0.05))
        pts = [(p[0] + (q[0] - p[0]) * k / n, p[1] + (q[1] - p[1]) * k / n) for k in range(n + 1)]
        x1, y1, x2, y2 = a["box"]
        return any(max(x1 - x, 0, x - x2) ** 2 + max(y1 - y, 0, y - y2) ** 2 <= hw * hw + 1e-6 for x, y in pts)
    (p, q, h1), (r, s, h2) = a["seg"], c["seg"]
    n = max(2, int(math.dist(p, q) / 0.05))
    for k in range(n + 1):
        x, y = p[0] + (q[0] - p[0]) * k / n, p[1] + (q[1] - p[1]) * k / n
        d = float(dist_field(dict(seg=(r, s, h2)), np.array([[x]]), np.array([[y]]))[0, 0])
        if d <= h1 + 1e-3:
            return True
    return False

def clusters(net):
    its = [it for it in items if it["net"] == net and it["kind"] in ("pad", "trk", "via")]
    parent = list(range(len(its)))
    def f(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]; i = parent[i]
        return i
    for i in range(len(its)):
        for j in range(i + 1, len(its)):
            if f(i) != f(j) and touching(its[i], its[j]):
                parent[f(i)] = f(j)
    groups = {}
    for i, it in enumerate(its):
        groups.setdefault(f(i), []).append(it)
    # only clusters with a pad matter
    return [g for g in groups.values() if any(x["kind"] == "pad" for x in g)]

def cells_of(group):
    out = set()
    for it in group:
        if it["kind"] == "pad":
            x1, y1, x2, y2 = it["box"]
            cx, cy = (x1 + x2) / 2, (y1 + y2) / 2
            r1, c1 = idx(x1 + 0.04, y1 + 0.04); r2, c2 = idx(x2 - 0.04, y2 - 0.04)
            for l in it["L"]:
                out.add((l,) + idx(cx, cy))
                for r in range(r1, r2 + 1):
                    for c in range(c1, c2 + 1):
                        out.add((l, r, c))
        else:
            (p, q, hw) = it["seg"]
            n = max(1, int(math.dist(p, q) / G))
            for l in it["L"]:
                for k in range(n + 1):
                    out.add((l,) + idx(p[0] + (q[0] - p[0]) * k / n, p[1] + (q[1] - p[1]) * k / n))
    return out

DIRS = [(0, 1, 1.0), (1, 0, 1.0), (0, -1, 1.0), (-1, 0, 1.0), (1, 1, 1.4142), (1, -1, 1.4142), (-1, 1, 1.4142), (-1, -1, 1.4142)]

def astar(starts, targets, hard, soft, vhard, vsoft, via_cost, allow_soft=True, maxn=8_000_000):
    tset = targets
    tp = np.array([(r, c) for _, r, c in list(targets)[:: max(1, len(targets) // 300)]])
    def h(r, c):
        d = np.abs(tp - (r, c))
        return float(np.min(np.maximum(d[:, 0], d[:, 1]) + 0.4142 * np.minimum(d[:, 0], d[:, 1])))
    g, parent, openq = {}, {}, []
    for s in starts:
        g[s] = 0.0; parent[s] = None
        heapq.heappush(openq, (h(s[1], s[2]), 0.0, s, -1))
    n = 0
    while openq:
        f, gc, cur, pdir = heapq.heappop(openq)
        if gc > g.get(cur, 1e18):
            continue
        if cur in tset:
            path = []
            while cur is not None:
                path.append(cur); cur = parent[cur]
            return path[::-1]
        n += 1
        if n > maxn:
            return None
        l, r, c = cur
        for k, (dr, dc, w) in enumerate(DIRS):
            rr, cc = r + dr, c + dc
            if not (0 <= rr < N and 0 <= cc < N):
                continue
            nb = (l, rr, cc)
            if hard[l, rr, cc] and nb not in tset:
                continue
            s = soft[l, rr, cc]
            if s and not allow_soft and nb not in tset:
                continue
            cost = w + (SOFT_PEN if s else 0) + (0.3 if (pdir >= 0 and k != pdir) else 0)
            ng = gc + cost
            if ng < g.get(nb, 1e18):
                g[nb] = ng; parent[nb] = cur
                heapq.heappush(openq, (ng + h(rr, cc), ng, nb, k))
        nl = 1 - l
        if not vhard[r, c] and not hard[nl, r, c]:
            vs = (vsoft[r, c] > 0) or soft[nl, r, c] > 0
            if allow_soft or not vs:
                nb = (nl, r, c)
                ng = gc + via_cost + (SOFT_PEN * 5 if vs else 0)
                if ng < g.get(nb, 1e18):
                    g[nb] = ng; parent[nb] = cur
                    heapq.heappush(openq, (ng + h(r, c), ng, nb, -1))
    return None

def pos(r, c):
    return (X0 + c * G, Y0 + r * G)

def path_geometry(path, w):
    segs, vias = [], []
    run = [path[0]]
    for a, c in zip(path, path[1:]):
        if a[0] != c[0]:
            vias.append(pos(a[1], a[2])); segs.append(run); run = [c]
        else:
            run.append(c)
    segs.append(run)
    out = []
    for run in segs:
        if len(run) < 2:
            continue
        pts = [run[0]]
        for k in range(1, len(run) - 1):
            d1 = (run[k][1] - run[k - 1][1], run[k][2] - run[k - 1][2])
            d2 = (run[k + 1][1] - run[k][1], run[k + 1][2] - run[k][2])
            if d1 != d2:
                pts.append(run[k])
        pts.append(run[-1])
        for a, c in zip(pts, pts[1:]):
            out.append((run[0][0], pos(a[1], a[2]), pos(c[1], c[2])))
    return out, vias

def violators(net, geo, vias, w):
    """unlocked other-net items that clash with the new geometry."""
    hits = set()
    probes = [(l, a, c, w / 2) for l, a, c in geo] + [(L, (x, y), (x, y), VIA_D / 2) for x, y in vias for L in (0, 1)]
    for it in items:
        if it["net"] == net or it["locked"]:
            continue
        for l, a, c, hw in probes:
            if l not in it["L"]:
                continue
            n = max(1, int(math.dist(a, c) / 0.05))
            xs_ = np.array([[a[0] + (c[0] - a[0]) * k / n for k in range(n + 1)]])
            ys_ = np.array([[a[1] + (c[1] - a[1]) * k / n for k in range(n + 1)]])
            if float(np.min(dist_field(it, xs_, ys_))) < hw + clr(net, it["net"]) - 0.001:
                hits.add(it["net"]); break
            if it["hole"] and hw == VIA_D / 2 and float(np.min(dist_field(it, xs_, ys_))) + (it["seg"][2] if "seg" in it else 0) < VIA_D + 0.25:
                hits.add(it["net"]); break
    return hits

REMOVED = set()
def rip(net):
    n = 0
    for it in [it for it in items if it["net"] == net and not it["locked"] and it["obj"] is not None]:
        REMOVED.add(it["obj"]); items.remove(it); n += 1
    return n

def emit(net, geo, vias, w):
    ni = b.FindNet(net)
    for l, (x1, y1), (x2, y2) in geo:
        t = pcbnew.PCB_TRACK(b)
        t.SetStart(VECTOR2I(FromMM(x1), FromMM(y1))); t.SetEnd(VECTOR2I(FromMM(x2), FromMM(y2)))
        t.SetWidth(FromMM(w)); t.SetLayer(LAYID[l]); t.SetNet(ni); b.Add(t)
        items.append(dict(kind="trk", L={l}, seg=((x1, y1), (x2, y2), w / 2), net=net, hole=False, locked=False,
                          obj=t.m_Uuid.AsString()))
    for x, y in vias:
        v = pcbnew.PCB_VIA(b)
        v.SetPosition(VECTOR2I(FromMM(x), FromMM(y))); v.SetWidth(FromMM(VIA_D)); v.SetDrill(FromMM(VIA_DR))
        v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu); v.SetNet(ni); b.Add(v)
        items.append(dict(kind="via", L={0, 1}, seg=((x, y), (x, y), VIA_D / 2), net=net, hole=True, locked=False,
                          obj=v.m_Uuid.AsString()))

def route_net(net, allow_soft=True):
    w = CLASS_W.get(net, 0.15)
    vc = VIA_COST.get(net, 60.0)
    total_len, total_v, victims = 0.0, 0, set()
    while True:
        cl = clusters(net)
        if len(cl) <= 1:
            return True, total_len, total_v, victims
        cl.sort(key=lambda g: -len(g))
        start = cells_of(cl[0])
        target = set().union(*[cells_of(g) for g in cl[1:]])
        hard, soft, vhard, vsoft = raster(net, w / 2)
        for (l, r, c) in start | target:
            hard[l, r, c] = False
        path = astar(start, target, hard, soft, vhard, vsoft, vc, allow_soft=False, maxn=1_500_000)
        if path is None and allow_soft:
            path = astar(start, target, hard, soft, vhard, vsoft, vc, allow_soft=True)
        if path is None:
            return False, total_len, total_v, victims
        geo, vias = path_geometry(path, w)
        hit = violators(net, geo, vias, w)
        for vn in hit:
            rip(vn)
        victims |= hit
        emit(net, geo, vias, w)
        total_len += sum(math.dist(a, c) for _, a, c in geo); total_v += len(vias)

t0 = time.time()
attempts = {}
done = []
for n in QUEUE:
    if not os.environ.get("NOPRERIP"):
        print(f"rip {n}: {rip(n)} items")
queue = list(QUEUE)
while queue:
    net = queue.pop(0)
    attempts[net] = attempts.get(net, 0) + 1
    if attempts[net] > 4:
        print("GIVING UP", net); continue
    ok, L, V, vict = route_net(net, allow_soft=(not os.environ.get('NORIP')) and ((net in QUEUE) or not os.environ.get('RIPONCE')))
    print(f"{'routed' if ok else 'FAILED'} {net}: {L:.1f} mm, {V} vias, ripped {sorted(vict)}  [{time.time() - t0:.0f}s]", flush=True)
    for v in vict:
        if v not in queue:
            queue.append(v)
    if not ok:
        queue.append(net)

b.Save(PCB)
# drop ripped items by uuid (pcbnew.BOARD.Remove is unstable in the KiCad 7 python API)
import re as _re
txt = open(PCB).read()
out, i, removed = [], 0, 0
pat = _re.compile(r"\n  \((segment|via|arc) ")
pos_ = 0
for m in pat.finditer(txt):
    pass
blocks = _re.split(r"(?=\n  \((?:segment|via|arc) )", txt)
keep = []
for blk in blocks:
    m = _re.match(r"\n  \((segment|via|arc) ", blk)
    if m:
        u = _re.search(r'\(uuid "?([0-9a-f-]+)"?\)', blk) or _re.search(r'\(tstamp "?([0-9a-f-]+)"?\)', blk)
        # a block may contain trailing non-track content after the closing paren; split at its end
        depth, end = 0, None
        for k, ch in enumerate(blk[1:], 1):
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    end = k + 1; break
        if u and u.group(1) in REMOVED:
            keep.append(blk[end:]); removed += 1
            continue
    keep.append(blk)
open(PCB, "w").write("".join(keep))
print("removed from file:", removed, "of", len(REMOVED))
b2 = pcbnew.LoadBoard(PCB)
pcbnew.ZONE_FILLER(b2).Fill(b2.Zones())
b2.Save(PCB)
print("saved")
