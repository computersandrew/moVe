"""Compare exported KiCad netlist against design.py intent; report single-node nets."""
import re, sys
from collections import defaultdict
from sexp import parse, find, findall
import design as D
from gen_sch_resolve import resolve_all

net = parse(open(sys.argv[1]).read())
nets = find(net, "nets")
actual = {}
for n in findall(nets, "net"):
    name = find(n, "name")[1].lstrip("/")
    if "/" in name and not name.startswith("unconnected") and not name.startswith("Net-("):
        name = name.split("/")[-1]
    nodes = {(find(x, "ref")[1], find(x, "pin")[1]) for x in findall(n, "node")}
    actual[name] = nodes

intent = defaultdict(set)
for ref, pin, netname in resolve_all():
    if netname is not None:
        intent[netname].add((ref, pin))

errors = 0
for name, nodes in intent.items():
    a = actual.get(name)
    if a is None:
        print("MISSING NET", name); errors += 1; continue
    if a != nodes:
        print("MISMATCH", name, "extra:", sorted(a - nodes)[:6], "missing:", sorted(nodes - a)[:6]); errors += 1
for name, nodes in actual.items():
    if name not in intent and not name.startswith("unconnected-") and not name.startswith("Net-("):
        print("UNEXPECTED NET", name, sorted(nodes)[:5]); errors += 1
    if len(nodes) == 1 and not name.startswith("unconnected-"):
        print("SINGLE-NODE NET", name, nodes)
unc = [n for n in actual if n.startswith("unconnected-") or n.startswith("Net-(")]
print(f"{len(intent)} intended nets, {len(actual)} netlist nets, {len(unc)} unconnected/anon, {errors} errors")
