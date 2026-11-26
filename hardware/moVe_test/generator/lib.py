"""Load KiCad symbol definitions (flattening 'extends')."""
import copy, os
from sexp import parse, find, findall, Sym

SYMDIR = os.environ.get("KICAD7_SYMBOL_DIR", "/usr/share/kicad/symbols")
_cache = {}

def _load_lib(lib):
    if lib not in _cache:
        path = lib if lib.endswith(".kicad_sym") else os.path.join(SYMDIR, lib + ".kicad_sym")
        tree = parse(open(path).read())
        _cache[lib] = {s[1]: s for s in findall(tree, "symbol")}
    return _cache[lib]

def get_symbol(lib_id, libfile=None):
    lib, name = lib_id.split(":")
    syms = _load_lib(libfile or lib)
    s = copy.deepcopy(syms[name])
    ext = find(s, "extends")
    if ext:
        parent = copy.deepcopy(syms[ext[1]])
        # replace properties with child's, keep parent's graphics/pins (renamed)
        child_props = {p[1]: p for p in findall(s, "property")}
        newsym = [Sym("symbol"), name]
        for item in parent[2:]:
            if isinstance(item, list) and item[0] == "property":
                newsym.append(child_props.pop(item[1], item))
            elif isinstance(item, list) and item[0] == "symbol":
                sub = copy.deepcopy(item)
                sub[1] = sub[1].replace(ext[1], name, 1)
                newsym.append(sub)
            else:
                newsym.append(item)
        for p in child_props.values():
            newsym.append(p)
        s = newsym
    return s

def pins(sym):
    """Return list of dicts: number, name, x, y (lib coords, Y up), angle, length, type, hidden, unit."""
    out = []
    for sub in findall(sym, "symbol"):
        # unit naming: NAME_U_S
        parts = sub[1].rsplit("_", 2)
        unit = int(parts[1]) if len(parts) == 3 else 1
        for p in findall(sub, "pin"):
            at = find(p, "at")
            out.append(dict(
                type=str(p[1]), number=find(p, "number")[1], name=find(p, "name")[1],
                x=float(at[1]), y=float(at[2]), angle=float(at[3]) if len(at) > 3 else 0.0,
                length=float(find(p, "length")[1]),
                hidden=any(x == "hide" for x in p if isinstance(x, Sym)),
                unit=unit))
    return out

def prop(sym, key):
    for p in findall(sym, "property"):
        if p[1] == key:
            return p[2]
    return None
