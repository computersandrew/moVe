"""Resolve design.py pin assignments to (ref, pin_number, net) using the library symbols."""
import re, os
import lib as L
import design as D

CUSTOM = os.path.join(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")), "moVe.kicad_sym")

def libsym(lib_id):
    if lib_id.startswith("moVe:"):
        return L.get_symbol(lib_id, CUSTOM)
    return L.get_symbol(lib_id)

def resolve_all():
    out = []
    for c in D.COMPS:
        if c.ref.startswith("#"):
            continue
        pins = L.pins(libsym(c.lib_id))
        if "@byname" in c.pins:
            byname = c.pins["@byname"]
            for p in pins:
                nm = re.split(r"[(/]", p["name"])[0]
                out.append((c.ref, p["number"], byname.get(nm)))
        else:
            for p in pins:
                out.append((c.ref, p["number"], c.pins.get(p["number"])))
    return out
