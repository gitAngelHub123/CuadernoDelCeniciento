#!/usr/bin/env python3
"""Lista los literales u8"..." de src/ que aún no tienen traducción (ni en ingles_gen.h ni en ingles_texto.h)."""
import re, glob, os, sys
src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")
lit = r'u8"((?:[^"\\]|\\.)*)"'
tengo = set()
for f in ("ingles_gen.h", "ingles_texto.h"):
    p = os.path.join(src, f)
    if os.path.exists(p):
        for m in re.finditer(r'\{u8"((?:[^"\\]|\\.)*)",\s*u8"', open(p, encoding="utf-8").read()):
            tengo.add(m.group(1))
falta = []
for f in sorted(glob.glob(src + "/*.h") + glob.glob(src + "/*.cpp")):
    if os.path.basename(f).startswith("ingles_"): continue
    for n, linea in enumerate(open(f, encoding="utf-8"), 1):
        for m in re.finditer(lit, linea):
            t = m.group(1)
            if t not in tengo and re.search(r"[A-Za-zÁ-ú]{2}", t):
                falta.append((os.path.basename(f), n, t))
vistos = set()
for f, n, t in falta:
    if t in vistos: continue
    vistos.add(t); print(f"{f}:{n}: {t}")
print(len(vistos), "sin traducir", file=sys.stderr)
sys.exit(1 if falta else 0)
