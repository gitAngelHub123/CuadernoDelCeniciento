#!/usr/bin/env python3
"""Recorta Noto Serif CJK SC (SIL OFL 1.1) a los caracteres chinos que usa el cuaderno.
Uso: generar_fuente_cjk.py [<NotoSerifCJK-VF.ttc>]   -> recursos/NotoSerifSC-cuaderno.ttf
Deja solo los glifos de src/chino_*.h, fija el peso (wght 500) y pasa los contornos CFF2 a TrueType
(stb_truetype de ImGui no lee CFF2). Volver a ejecutarlo si cambian los textos en chino."""
import sys, os, glob
from fontTools.ttLib import TTCollection, newTable
from fontTools.varLib import instancer
from fontTools import subset
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.ttGlyphPen import TTGlyphPen

raiz = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
ttc = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/google-noto-serif-cjk-vf-fonts/NotoSerifCJK-VF.ttc"
salida = os.path.join(raiz, "recursos", "NotoSerifSC-cuaderno.ttf")

chars = set()
for f in glob.glob(os.path.join(raiz, "src", "chino_*.h")):
    chars |= {ord(c) for c in open(f, encoding="utf-8").read() if ord(c) >= 0x2E80}

fuente = next(f for f in TTCollection(ttc) if "SC" in f["name"].getDebugName(4))
# primero recortar (la fuente completa tiene ~65 000 glifos), luego fijar el peso
op = subset.Options(); op.name_IDs = ["*"]; op.name_languages = ["*"]; op.notdef_outline = True; op.layout_features = []
s = subset.Subsetter(op); s.populate(unicodes=sorted(chars)); s.subset(fuente)
fuente = instancer.instantiateVariableFont(fuente, {"wght": 500})

# CFF2 -> glyf (cuadráticas)
orden = fuente.getGlyphOrder()
gs = fuente.getGlyphSet()
glyf = newTable("glyf"); glyf.glyphOrder = orden; glyf.glyphs = {}
for n in orden:
    pen = TTGlyphPen(None)
    gs[n].draw(Cu2QuPen(pen, 1.0, reverse_direction=True))
    glyf.glyphs[n] = pen.glyph()
fuente["glyf"] = glyf
fuente["loca"] = newTable("loca")
del fuente["CFF2"]
for t in ("VORG", "HVAR", "MVAR", "VVAR", "STAT"):
    if t in fuente: del fuente[t]
maxp = fuente["maxp"]; maxp.tableVersion = 0x00010000
for k in ("maxZones", "maxTwilightPoints", "maxStorage", "maxFunctionDefs", "maxInstructionDefs",
          "maxStackElements", "maxSizeOfInstructions", "maxComponentElements"):
    setattr(maxp, k, 0)
maxp.maxZones = 1
for k in ("maxPoints", "maxContours", "maxCompositePoints", "maxCompositeContours", "maxComponentDepth"):
    setattr(maxp, k, 0)
fuente["head"].glyphDataFormat = 0
fuente.sfntVersion = "\x00\x01\x00\x00"
fuente["post"].formatType = 3.0
fuente.save(salida)
print(f"{len(chars)} caracteres -> {salida} ({os.path.getsize(salida)} B)")
