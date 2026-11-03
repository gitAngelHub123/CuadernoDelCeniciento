#!/usr/bin/env python3
"""Genera src/misiones_gen.h a partir de misiones.json (máquinas de estado extraídas de los EMEVD).

Solo números (flags, mapas). Los textos se escriben en el mod.
"""
import json, os, collections
AQUI = os.path.dirname(os.path.abspath(__file__))
d = json.load(open(os.path.join(AQUI, "misiones.json")))
BLOQUES = [1020, 1040, 1060, 1100, 1120, 1140, 1200, 1220, 1280, 1300, 1340, 1360, 1380, 1480]
MAPAS = {"m30_00_00_00": 1, "m30_01_00_00": 2, "m31_00_00_00": 3, "m32_00_00_00": 4, "m33_00_00_00": 5, "m34_01_00_00": 6,
         "m35_00_00_00": 7, "m37_00_00_00": 8, "m38_00_00_00": 9, "m39_00_00_00": 10, "m40_00_00_00": 11, "m41_00_00_00": 12,
         "m45_00_00_00": 13, "m50_00_00_00": 14, "m51_00_00_00": 15, "m51_01_00_00": 15}

fases, trans = [], []
for b in BLOQUES:
    for k, mapas in d["presencia"].items():
        k = int(k)
        if b <= k < b + 15:
            fases.append((k, MAPAS.get(mapas[0], 0)))
    vistos = set()
    for t in d["transiciones"].get(str(b), []):
        conds = [(f, s) for f, s in t["flags"] if f != 0][:4]
        for desde in (t["desde"] or [0]):
            clave = (desde, t["hacia"], tuple(conds))
            if clave in vistos: continue
            vistos.add(clave)
            trans.append((desde, t["hacia"], conds))

# profundidad de cada fase (para "paso k de N"): BFS desde la fase inicial del bloque
prof = {}
for b in BLOQUES:
    sig = collections.defaultdict(set)
    for de, ha, c in trans:
        if b <= ha < b + 15:
            sig[de if de else b].add(ha)
    cola, prof[b] = [b], 0
    while cola:
        x = cola.pop(0)
        for y in sig[x]:
            if y not in prof: prof[y] = prof[x] + 1; cola.append(y)

with open(os.path.join(AQUI, "..", "src", "misiones_gen.h"), "w") as f:
    f.write("// GENERADO por herramientas/generar_misiones.py a partir de los EMEVD del juego. No editar a mano.\n#pragma once\n#include <cstdint>\n")
    f.write("struct FaseLugar { uint16_t fase; uint8_t mapa; };\n")
    f.write("struct Transicion { uint16_t desde, hacia; uint32_t cond[4]; uint8_t estado[4]; };\n")
    f.write("struct FaseProf { uint16_t fase; uint8_t prof; };\n")
    f.write("static const FaseLugar FASES_LUGAR[] = {\n" + "".join(f"    {{{k}, {m}}},\n" for k, m in sorted(fases)) + "};\n")
    f.write("static const Transicion TRANSICIONES[] = {\n")
    for de, ha, c in trans:
        cs = [x for x, _ in c] + [0] * (4 - len(c)); es = [s for _, s in c] + [0] * (4 - len(c))
        f.write(f"    {{{de}, {ha}, {{{', '.join(map(str, cs))}}}, {{{', '.join(map(str, es))}}}}},\n")
    f.write("};\n")
    f.write("static const FaseProf FASES_PROF[] = {\n" + "".join(f"    {{{k}, {v}}},\n" for k, v in sorted(prof.items())) + "};\n")
print(len(fases), "fases con lugar,", len(trans), "transiciones,", len(prof), "profundidades")
