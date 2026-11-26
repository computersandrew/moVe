"""Delete track/via items from a .kicad_pcb by tstamp (text filter; BOARD.Remove is unstable in KiCad 7 python)."""
import re, sys
def delete(pcb, uuids):
    txt = open(pcb).read()
    lines = txt.split("\n")
    out, n = [], 0
    for ln in lines:
        s = ln.lstrip()
        if s.startswith("(segment ") or s.startswith("(via ") or s.startswith("(arc "):
            m = re.search(r"\(tstamp ([0-9a-f-]+)\)", ln)
            if m and m.group(1) in uuids:
                n += 1; continue
        out.append(ln)
    open(pcb, "w").write("\n".join(out))
    return n
if __name__ == "__main__":
    print(delete(sys.argv[1], set(sys.argv[2:])))
