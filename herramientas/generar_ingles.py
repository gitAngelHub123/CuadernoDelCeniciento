#!/usr/bin/env python3
"""Empareja textos oficiales es-ES con otro idioma del juego (por id de FMG) para los nombres que usa el mod.
Uso: generar_ingles.py <carpeta con msg__spaes__*.msgbnd y msg__<idioma>__*.msgbnd> <salida.json> [<cabecera.h> [<idioma> <TABLA>]]
     inglés (por defecto): idioma engus, tabla INGLES_GEN -> src/ingles_gen.h
     chino simplificado:   idioma zhocn, tabla CHINO_GEN  -> src/chino_gen.h
Solo se guardan los pares cuyo texto español aparece literalmente en src/. Nunca se publica lo extraído."""
import sys, os, re, json, glob, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import souls
carpeta, salida = sys.argv[1], sys.argv[2]
lengua = sys.argv[4] if len(sys.argv) > 4 else "engus"
tabla = sys.argv[5] if len(sys.argv) > 5 else "INGLES_GEN"
src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")
usados = set()
for f in glob.glob(src + "/*.h") + glob.glob(src + "/*.cpp"):
    if re.match(r"(ingles|chino)_", os.path.basename(f)): continue
    for m in re.finditer(r'u8"((?:[^"\\]|\\.)*)"', open(f, encoding="utf-8").read()):
        usados.add(m.group(1))
pares = collections.defaultdict(collections.Counter)
for nombre in ("item_dlc2", "menu_dlc2", "item_dlc1", "menu_dlc1"):
    es = souls.fmgs_of(open(f"{carpeta}/msg__spaes__{nombre}.msgbnd", "rb").read())
    otro = souls.fmgs_of(open(f"{carpeta}/msg__{lengua}__{nombre}.msgbnd", "rb").read())
    for fm, d in es.items():
        for i, t in d.items():
            e = otro.get(fm, {}).get(i)
            if isinstance(t, str) and e and t in usados and t != e:
                pares[t][(e, fm)] += 1
res = {}
for t, c in pares.items():
    res[t] = [{"tr": e, "fmg": fm, "n": n} for (e, fm), n in c.most_common()]
json.dump(res, open(salida, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
if len(sys.argv) > 3:
    prio = ["NPC名", "FDP_メニューテキスト", "アイテム名", "武器名", "防具名"]
    def clave(x):
        return (prio.index(x["fmg"]) if x["fmg"] in prio else 99, -x["n"])
    def c(t): return '"' + t.replace("\\", "\\\\").replace('"', '\\"') + '"'
    # el nombre del objeto, no el del menú
    t = "Horno de transposición"
    if t in res:
        obj = [x for x in res[t] if x["fmg"] == "アイテム名"]
        if lengua == "engus": res[t].insert(0, {"tr": "Transposing Kiln", "fmg": "NPC名", "n": 99})
        elif obj: res[t].insert(0, dict(obj[0], fmg="NPC名"))
    with open(sys.argv[3], "w", encoding="utf-8") as o:
        o.write(f"// GENERADO por herramientas/generar_ingles.py: nombres oficiales es-ES -> {lengua} (textos del juego, solo los nombres que usa el mod).\n"
                "#pragma once\n#ifndef PAR_IDIOMA\n#define PAR_IDIOMA\nstruct ParIdioma { const char* es; const char* tr; };\n#endif\n"
                f"static const ParIdioma {tabla}[] = {{\n")
        for t in sorted(res):
            o.write("    {u8%s, u8%s},\n" % (c(t), c(sorted(res[t], key=clave)[0]["tr"])))
        o.write("};\n")
print(len(usados), "literales; emparejados:", len(res))
