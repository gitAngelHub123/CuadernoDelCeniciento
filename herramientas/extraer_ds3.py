#!/usr/bin/env python3
"""Extrae archivos concretos de los Data*.bdt de Dark Souls 3 (solo para uso local, nunca se publican).

Uso:  extraer_ds3.py <salida> <ruta_virtual> [<ruta_virtual> ...]
      ej. /msg/spaes/item_dlc2.msgbnd.dcx
Formato BHD5/claves según BinderTool (Atvaark, referencia).
"""
import json, struct, sys, os, zlib
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES

GAME = os.environ.get("DS3_GAME", os.path.expanduser("~/.local/share/Steam/steamapps/common/DARK SOULS III/Game"))
KEYS = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "claves_bhd_ds3.json")))
ARCH = ["Data1", "Data2", "Data3", "Data4", "Data5", "DLC1", "DLC2"]

def h(path, prime=37):
    v = 0
    for c in path.replace("\\", "/").lower():
        v = (v * prime + ord(c)) & 0xFFFFFFFF
    return v

def rsa_decrypt(data, pem):
    k = RSA.import_key(pem)
    n, e = k.n, k.e
    ib = (k.size_in_bits() + 7) // 8
    ob = ib - 1
    out = bytearray()
    for i in range(0, len(data), ib):
        c = int.from_bytes(data[i:i+ib], "big")
        out += pow(c, e, n).to_bytes(ob, "big")
    return bytes(out)

def load_bhd(arch):
    raw = open(f"{GAME}/{arch}.bhd", "rb").read()
    if raw[:4] != b"BHD5":
        key = KEYS.get(arch.capitalize() if arch.startswith("Data") else arch.capitalize())
        key = KEYS.get({"DLC1": "Dlc1", "DLC2": "Dlc2"}.get(arch, arch))
        raw = rsa_decrypt(raw, key)
    assert raw[:4] == b"BHD5", arch
    nb, boff = struct.unpack_from("<ii", raw, 0x10)
    entries = {}
    for b in range(nb):
        cnt, eoff = struct.unpack_from("<ii", raw, boff + b * 8)
        for i in range(cnt):
            o = eoff + i * 0x28
            fh, psz, foff, shoff, aoff, fsz = struct.unpack_from("<IIqqqq", raw, o)
            aes = None
            if aoff:
                key = raw[aoff:aoff+16]
                rc = struct.unpack_from("<i", raw, aoff + 16)[0]
                rng = [struct.unpack_from("<qq", raw, aoff + 20 + j * 16) for j in range(rc)]
                aes = (key, rng)
            entries[fh] = (psz, foff, fsz, aes)
    return entries

def read_file(arch, ent):
    psz, foff, fsz, aes = ent
    with open(f"{GAME}/{arch}.bdt", "rb") as f:
        f.seek(foff); data = bytearray(f.read(psz))
    if aes:
        c = AES.new(aes[0], AES.MODE_ECB)
        for s, e in aes[1]:
            if s == -1 or e == -1 or s >= e: continue
            data[s:e] = c.decrypt(bytes(data[s:e]))
    return bytes(data[:fsz]) if fsz else bytes(data)

def dcx(data):
    if data[:4] != b"DCX\0": return data
    fmt = data[0x28:0x2C]
    usz, csz = struct.unpack_from(">II", data, 0x1C)
    if fmt == b"DFLT":
        return zlib.decompress(data[0x4C:0x4C+csz])
    raise ValueError(f"DCX {fmt} no soportado")

_cache = {}
def find(path):
    hv = h(path)
    for a in ARCH:
        if a not in _cache:
            try: _cache[a] = load_bhd(a)
            except Exception as ex: print("aviso", a, ex, file=sys.stderr); _cache[a] = {}
        if hv in _cache[a]:
            return a, _cache[a][hv]
    return None, None

if __name__ == "__main__":
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    for p in sys.argv[2:]:
        a, e = find(p)
        if not a: print("NO:", p); continue
        d = dcx(read_file(a, e))
        dst = os.path.join(out, p.strip("/").replace("/", "__").removesuffix(".dcx"))
        open(dst, "wb").write(d)
        print(f"ok {a}: {p} -> {os.path.basename(dst)} ({len(d)} B, {d[:4]})")
