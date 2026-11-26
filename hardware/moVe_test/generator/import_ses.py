"""Import a Freerouting .ses into a KiCad 7 board headlessly (pcbnew.ImportSpecctraSES needs the GUI in v7)."""
import re, sys
import pcbnew
from pcbnew import VECTOR2I
from sexp import parse, find, findall

PCB, SES = sys.argv[1], sys.argv[2]
POURS = "--no-pours" not in sys.argv
b = pcbnew.LoadBoard(PCB)
ses = parse(open(SES).read())
routes = find(ses, "routes")
res = find(routes, "resolution")
scale = {"um": 1e3, "mm": 1e6, "mil": 25400.0}[str(res[1])] / float(res[2])   # nm per unit

def pt(x, y):
    return VECTOR2I(int(round(float(x) * scale)), int(round(-float(y) * scale)))

layers = {"F.Cu": pcbnew.F_Cu, "B.Cu": pcbnew.B_Cu, "In1.GND": pcbnew.In1_Cu, "In2.3V3": pcbnew.In2_Cu}

existing = {(t.GetPosition().x // 1000, t.GetPosition().y // 1000) for t in b.GetTracks() if t.GetClass() == "PCB_VIA"}
fixed_segs = {(t.GetStart().x // 1000, t.GetStart().y // 1000, t.GetEnd().x // 1000, t.GetEnd().y // 1000)
              for t in b.GetTracks() if t.GetClass() == "PCB_TRACK"}

ntr = nvia = 0
for net in findall(find(routes, "network_out"), "net"):
    ni = b.FindNet(str(net[1]))
    if ni is None:
        print("unknown net", net[1]); continue
    for w in findall(net, "wire"):
        path = find(w, "path")
        if path is None:
            continue
        layer, width = layers[str(path[1])], float(path[2]) * scale
        coords = path[3:]
        pts = [pt(coords[i], coords[i + 1]) for i in range(0, len(coords) - 1, 2)]
        for a, c in zip(pts, pts[1:]):
            if (a.x // 1000, a.y // 1000, c.x // 1000, c.y // 1000) in fixed_segs or \
               (c.x // 1000, c.y // 1000, a.x // 1000, a.y // 1000) in fixed_segs:
                continue
            t = pcbnew.PCB_TRACK(b)
            t.SetStart(a); t.SetEnd(c); t.SetWidth(int(width)); t.SetLayer(layer); t.SetNet(ni)
            b.Add(t); ntr += 1
    for v in findall(net, "via"):
        m = re.search(r"_(\d+):(\d+)_um", str(v[1]))
        dia, drill = int(m.group(1)) * 1000, int(m.group(2)) * 1000
        pos = pt(v[2], v[3])
        if (pos.x // 1000, pos.y // 1000) in existing:
            continue
        via = pcbnew.PCB_VIA(b)
        via.SetPosition(pos)
        via.SetWidth(dia); via.SetDrill(drill)
        via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
        via.SetNet(ni)
        b.Add(via); nvia += 1

import math
def circle_poly(cx, cy, r, n=128):
    ch = pcbnew.SHAPE_LINE_CHAIN()
    for i in range(n):
        a = 2 * math.pi * i / n
        ch.Append(pcbnew.FromMM(cx + r * math.cos(a)), pcbnew.FromMM(cy + r * math.sin(a)))
    ch.SetClosed(True)
    return ch
gnd = b.FindNet("GND")
for layer in ((pcbnew.F_Cu, pcbnew.B_Cu) if POURS else ()):
    z = pcbnew.ZONE(b)
    z.SetLayer(layer); z.SetNet(gnd)
    z.Outline().AddOutline(circle_poly(150, 100, 4.2 * 25.4 / 2 - 0.5))
    z.SetLocalClearance(pcbnew.FromMM(0.3)); z.SetMinThickness(pcbnew.FromMM(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(pcbnew.FromMM(0.3)); z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.4))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    b.Add(z)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
b.Save(PCB)
print(f"imported {ntr} segments, {nvia} vias")
