#!/usr/bin/env python3
"""Busca patrones AOB en DarkSoulsIII.exe y resuelve el destino RIP-relativo (RVA)."""
import re, struct, sys
import os
EXE = os.path.join(os.environ.get("DS3_GAME", os.path.expanduser("~/.local/share/Steam/steamapps/common/DARK SOULS III/Game")), "DarkSoulsIII.exe")
d = open(EXE, "rb").read()
pe = struct.unpack_from("<I", d, 0x3c)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
optsz = struct.unpack_from("<H", d, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + optsz + i * 40
    name = d[o:o+8].rstrip(b"\0").decode()
    vsz, va, rsz, raw = struct.unpack_from("<IIII", d, o + 8)
    secs.append((name, va, vsz, raw, rsz))
def off2rva(off):
    for n, va, vsz, raw, rsz in secs:
        if raw <= off < raw + rsz: return va + off - raw
def rva2off(rva):
    for n, va, vsz, raw, rsz in secs:
        if va <= rva < va + rsz: return raw + rva - va
def compile_aob(s):
    return re.compile(b"".join(b"." if t in ("?", "??") else re.escape(bytes([int(t, 16)])) for t in s.split()), re.S)
def scan(aob, rel_off=None, ins_len=None, limit=5):
    out = []
    for m in compile_aob(aob).finditer(d):
        rva = off2rva(m.start())
        tgt = None
        if rel_off is not None:
            disp = struct.unpack_from("<i", d, m.start() + rel_off)[0]
            tgt = rva + ins_len + disp
        out.append((rva, tgt))
        if len(out) >= limit: break
    return out
if __name__ == "__main__":
    for n, va, vsz, raw, rsz in secs: print(f"{n:8} va={va:#x} vsz={vsz:#x} raw={raw:#x} rsz={rsz:#x}")
