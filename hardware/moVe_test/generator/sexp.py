"""Minimal S-expression parser/writer for KiCad files."""
import re

class Sym(str):
    """Unquoted atom."""
    pass

_tok = re.compile(r'\s*(?:(\()|(\))|"((?:[^"\\]|\\.)*)"|([^\s()"]+))', re.S)

def parse(text):
    stack = [[]]
    pos = 0
    n = len(text)
    while pos < n:
        m = _tok.match(text, pos)
        if not m:
            if text[pos:].strip() == '':
                break
            raise ValueError("parse error at %d: %r" % (pos, text[pos:pos+40]))
        pos = m.end()
        if m.group(1):
            stack.append([])
        elif m.group(2):
            lst = stack.pop()
            stack[-1].append(lst)
        elif m.group(3) is not None:
            stack[-1].append(m.group(3).replace('\\"', '"').replace('\\\\', '\\'))
        else:
            stack[-1].append(Sym(m.group(4)))
    return stack[0][0] if len(stack[0]) == 1 else stack[0]

def q(s):
    return '"' + str(s).replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'

def dump(e, indent=0):
    """Serialize. Lists -> (..), Sym -> bare, str -> quoted, numbers -> repr."""
    if isinstance(e, list):
        if not e:
            return "()"
        simple = all(not isinstance(x, list) for x in e)
        if simple:
            return "(" + " ".join(dump(x) for x in e) + ")"
        out = "(" + dump(e[0])
        for x in e[1:]:
            if isinstance(x, list):
                out += "\n" + "  " * (indent + 1) + dump(x, indent + 1)
            else:
                out += " " + dump(x)
        return out + "\n" + "  " * indent + ")"
    if isinstance(e, Sym):
        return str(e)
    if isinstance(e, bool):
        return "yes" if e else "no"
    if isinstance(e, float):
        s = ("%.4f" % e).rstrip("0").rstrip(".")
        return s if s not in ("-0", "") else "0"
    if isinstance(e, int):
        return str(e)
    return q(e)

def find(e, key):
    for x in e:
        if isinstance(x, list) and x and x[0] == key:
            return x
    return None

def findall(e, key):
    return [x for x in e if isinstance(x, list) and x and x[0] == key]
