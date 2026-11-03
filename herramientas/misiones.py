#!/usr/bin/env python3
"""Extrae de los EMEVD de DS3 las máquinas de estado de las misiones de NPCs.

Para cada bloque de 20 flags (1000-1999): transiciones fase -> fase con sus condiciones,
y en qué mapas se usa cada fase (para saber dónde aparece el NPC).
Salida: misiones.json (solo datos derivados: números de flag y mapas, sin texto del juego).
"""
import sys, os, json, glob, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emevd import cargar_emedf, leer, decodificar

DIR = sys.argv[1]
I = cargar_emedf()

def instrucciones(ruta):
    for eid, ins, pars in leer(ruta):
        yield eid, [decodificar(I, b, i, d) for b, i, d in ins]

def es_fase(f): return 1000 <= f < 2000 and (f % 20) < 15

transiciones = collections.defaultdict(list)   # bloque -> [(desde, hacia, [(flag, estado)], [otras], mapa, evento)]
uso_mapa = collections.defaultdict(set)         # flag de fase -> mapas donde se consulta

for ruta in sorted(glob.glob(os.path.join(DIR, "event__m*.emevd"))):
    mapa = os.path.basename(ruta)[7:-6]
    for eid, ins in instrucciones(ruta):
        grupos = collections.defaultdict(list)
        for k, (nom, vals) in enumerate(ins):
            v = dict(vals)
            # condiciones por grupo
            if nom == "IF Event Flag":
                grupos[v["Result Condition Group"]].append(("flag", v["Target Event Flag ID"], v["Desired Flag State"]))
                f = v["Target Event Flag ID"]
                if es_fase(f): uso_mapa[f].add(mapa)
            elif nom.startswith("IF ") and "Result Condition Group" in v:
                grupos[v["Result Condition Group"]].append(("otro", nom, tuple(x for a, x in vals if a != "Result Condition Group")))
            elif "Event Flag" in nom and nom.startswith(("SKIP IF", "GOTO IF", "END IF")):
                f = v.get("Target Event Flag ID")
                if isinstance(f, int) and es_fase(f): uso_mapa[f].add(mapa)
            # transición: "Set Event Flag" a una fase justo tras un SKIP por grupo
            if nom in ("Set Network-Connected Event Flag", "Set Event Flag") and v.get("Flag State") == 1:
                hacia = v.get("Event Flag ID")
                if not (isinstance(hacia, int) and es_fase(hacia)): continue
                # buscar hacia atrás el SKIP IF Condition Group que protege este bloque
                for j in range(k - 1, max(-1, k - 4), -1):
                    n2, v2 = ins[j]; d2 = dict(v2)
                    if n2.startswith("SKIP IF Condition Group"):
                        g = d2.get("Target Condition Group")
                        conds = grupos.get(g, [])
                        desde = [c[1] for c in conds if c[0] == "flag" and es_fase(c[1]) and c[2] == 1 and c[1] // 20 == hacia // 20]
                        resto = [(c[1], c[2]) for c in conds if c[0] == "flag" and not (es_fase(c[1]) and c[1] // 20 == hacia // 20)]
                        otros = [(c[1], c[2]) for c in conds if c[0] == "otro"]
                        transiciones[hacia // 20 * 20].append({"desde": desde, "hacia": hacia, "flags": resto,
                                                               "otros": [list(o) for o in otros], "mapa": mapa, "evento": eid})
                        break

# Presencia: patrón "GOTO IF Event Flag <fase> ON" (uno o varios seguidos) justo antes de
# "Change Character Enable State = desactivado": el NPC está en ese mapa durante esas fases.
presencia = collections.defaultdict(set)
for ruta in sorted(glob.glob(os.path.join(DIR, "event__m*.emevd"))):
    mapa = os.path.basename(ruta)[7:-6]
    for eid, ins in instrucciones(ruta):
        for k, (n, vals) in enumerate(ins):
            if n != "Change Character Enable State" or dict(vals).get("Disabled/Enabled") != 0: continue
            j = k - 1
            while j >= 0:
                n2, v2 = ins[j]; d2 = dict(v2)
                f = d2.get("Target Event Flag ID")
                if n2 == "GOTO IF Event Flag" and d2.get("Desired Flag State") == 1 and isinstance(f, int) and es_fase(f):
                    presencia[f].add(mapa); j -= 1
                elif n2 == "GOTO IF Batch Event Flags" and d2.get("Desired Flag State") == 1:
                    for x in range(d2.get("Starting Target Event Flag ID", 0), d2.get("Ending Target Event Flag ID", -1) + 1):
                        if es_fase(x): presencia[x].add(mapa)
                    j -= 1
                else: break

salida = {"presencia": {str(k): sorted(v) for k, v in sorted(presencia.items())}, "transiciones": {str(k): v for k, v in sorted(transiciones.items())},
          "mapas": {str(k): sorted(v) for k, v in sorted(uso_mapa.items())}}
json.dump(salida, open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "misiones.json"), "w"), indent=1, ensure_ascii=False)
for b, t in sorted(transiciones.items()):
    print(b, len(t), "transiciones")
