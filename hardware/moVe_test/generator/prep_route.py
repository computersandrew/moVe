"""Pre-routing pipeline for a board copy: placement -> fanout -> ICM fix -> project (0.127 clr) -> DSN."""
import json, os, subprocess, sys
D = sys.argv[1]
os.makedirs(D, exist_ok=True)
PCB = os.path.join(D, "moVe_test.kicad_pcb")
PRJ = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
subprocess.run(f"cp -r {PRJ}/moVe.pretty {PRJ}/fp-lib-table {D}/", shell=True, check=True)
env = dict(os.environ, BOARD_OUT=PCB)
subprocess.run(["/usr/bin/python3", "gen_pcb.py"], env=env, check=True, stdout=subprocess.DEVNULL)
print(subprocess.run(["/usr/bin/python3", "fanout.py", PCB], capture_output=True, text=True).stdout.strip().splitlines()[-1])
import pcbnew
from pcbnew import ToMM, FromMM, VECTOR2I
b = pcbnew.LoadBoard(PCB)
u7 = b.FindFootprintByReference("U7")
pads = {p.GetNumber(): ToMM(p.GetPosition()) for p in u7.Pads()}
p11 = pads["11"]; tgt = (p11[0] + 0.25, p11[1] + 1.4); old = None
for t in b.GetTracks():
    if t.GetNetname() == "GND" and t.GetClass() == "PCB_TRACK":
        s = ToMM(t.GetStart())
        if abs(s[0] - p11[0]) < 0.01 and abs(s[1] - p11[1]) < 0.01:
            old = ToMM(t.GetEnd()); t.SetEnd(VECTOR2I(FromMM(tgt[0]), FromMM(tgt[1]))); t.SetWidth(FromMM(0.15))
for t in b.GetTracks():
    if old and t.GetClass() == "PCB_VIA" and t.GetNetname() == "GND":
        x, y = ToMM(t.GetPosition())
        if abs(x - old[0]) < 0.01 and abs(y - old[1]) < 0.01:
            t.SetPosition(VECTOR2I(FromMM(tgt[0]), FromMM(tgt[1])))
def trk(a, c, net):
    t = pcbnew.PCB_TRACK(b); t.SetStart(VECTOR2I(FromMM(a[0]), FromMM(a[1]))); t.SetEnd(VECTOR2I(FromMM(c[0]), FromMM(c[1])))
    t.SetWidth(FromMM(0.15)); t.SetLayer(pcbnew.F_Cu); t.SetNet(b.FindNet(net)); t.SetLocked(True); b.Add(t)
trk(pads["8"], pads["9"], "+1V8")
trk(pads["10"], (pads["10"][0], 87.98), "ICM_REGOUT")
b.Save(PCB)
src = open("route.py").read()
ns = {}
exec(src.split('if STAGE in ("all", "dsn"):')[0].replace('STAGE = sys.argv[1] if len(sys.argv) > 1 else "all"', 'STAGE="x"'), ns)
ns["PRO"] = os.path.join(D, "moVe_test.kicad_pro"); ns["write_project"]()
pro = json.load(open(ns["PRO"]))
for c in pro["net_settings"]["classes"]:
    if c["name"] in ("Default", "USB", "PowerNarrow"):
        c["clearance"] = 0.127
pro["board"]["design_settings"]["rules"]["min_clearance"] = 0.127
json.dump(pro, open(ns["PRO"], "w"), indent=2)
b = pcbnew.LoadBoard(PCB)
pcbnew.WriteDRCReport(b, os.path.join(D, "pre.rpt"), pcbnew.EDA_UNITS_MILLIMETRES, True)
print("dsn", pcbnew.ExportSpecctraDSN(b, os.path.join(D, "moVe_test.dsn")))
