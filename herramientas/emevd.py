#!/usr/bin/env python3
"""Lector mínimo de EMEVD de DS3 (formato según SoulsFormats) con nombres de instrucciones de EMEDF."""
import json, struct, os

EMEDF = os.path.expanduser("~/Proyectos/CuadernoDS3/referencias/SoulsRandomizers/dist/Base/ds3-common.emedf.json")
_TAM = {0: ("B", 1), 1: ("H", 2), 2: ("I", 4), 3: ("b", 1), 4: ("h", 2), 5: ("i", 4), 6: ("f", 4), 8: ("I", 4)}

def cargar_emedf():
    import re
    texto = "\n".join(l for l in open(EMEDF, encoding="utf-8").read().splitlines() if not l.strip().startswith("//"))
    texto = re.sub(r",(\s*[}\]])", r"\1", texto)   # comas finales que deja quitar comentarios
    d = json.loads(texto)
    instr = {}
    for c in d["main_classes"]:
        for i in c["instrs"]:
            instr[(c["index"], i["index"])] = (i["name"], [(a["name"], a["type"]) for a in i["args"]])
    return instr

def leer(ruta):
    b = open(ruta, "rb").read()
    assert b[:4] == b"EVD\0"
    q = lambda o: struct.unpack_from("<q", b, o)[0]
    nev, oev = q(0x10), q(0x18)
    oins, opar, oarg = q(0x28), q(0x58), q(0x78)
    eventos = []
    for e in range(nev):
        o = oev + e * 0x30
        eid, nins, ioff, npar, poff = struct.unpack_from("<qqqqq", b, o)
        ins = []
        for k in range(nins):
            io = oins + ioff + k * 0x20
            bank, iid, alen, aoff, loff = struct.unpack_from("<iiqqq", b, io)
            ins.append((bank, iid, b[oarg + aoff: oarg + aoff + alen]))
        pars = []
        for k in range(npar):
            po = opar + poff + k * 0x20
            ii, tgt, src, cnt = struct.unpack_from("<qqqi", b, po)
            pars.append((ii, tgt, src, cnt))
        eventos.append((eid, ins, pars))
    return eventos

def decodificar(instr, bank, iid, data):
    nombre, args = instr.get((bank, iid), (f"{bank}[{iid:02d}]", []))
    vals, off = [], 0
    for an, at in args:
        f, n = _TAM.get(at, ("i", 4))
        off = (off + n - 1) // n * n
        if off + n > len(data): break
        vals.append((an, struct.unpack_from("<" + f, data, off)[0]))
        off += n
    return nombre, vals
