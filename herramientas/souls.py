"""Lectores mínimos de formatos de FromSoftware (BND4, FMG v2) para uso local."""
import struct

def bnd4(data):
    assert data[:4] == b"BND4", data[:4]
    count = struct.unpack_from("<i", data, 0x0C)[0]
    fh_size = struct.unpack_from("<q", data, 0x20)[0]
    unicode = data[0x30] == 1
    fmt = data[0x31]
    files = []
    for i in range(count):
        o = 0x40 + i * fh_size
        csz = struct.unpack_from("<q", data, o + 8)[0]
        p = o + 16
        if fh_size >= 0x24:          # tiene tamaño sin comprimir
            p += 8
        if fh_size in (0x24, 0x20, 0x1C) or fh_size < 0x28:
            doff = struct.unpack_from("<I", data, p)[0]; p += 4
        else:
            doff = struct.unpack_from("<q", data, p)[0]; p += 8
        fid = struct.unpack_from("<i", data, p)[0]; p += 4
        noff = struct.unpack_from("<I", data, p)[0]
        if unicode:
            e = noff
            while data[e:e+2] != b"\0\0": e += 2
            name = data[noff:e].decode("utf-16le")
        else:
            name = data[noff:data.index(b"\0", noff)].decode("shift_jis", "replace")
        files.append((fid, name, data[doff:doff+csz]))
    return files

def fmg(data):
    """Devuelve {id: texto} de un FMG de DS3 (versión 2, offsets de 64 bits)."""
    assert data[2] == 2, f"FMG versión {data[2]}"
    groups, nstr = struct.unpack_from("<ii", data, 0x0C)
    soff = struct.unpack_from("<q", data, 0x18)[0]
    out = {}
    for g in range(groups):
        idx, first, last, _ = struct.unpack_from("<iiii", data, 0x28 + g * 16)
        for k, tid in enumerate(range(first, last + 1)):
            so = struct.unpack_from("<q", data, soff + (idx + k) * 8)[0]
            if so <= 0:
                continue
            e = so
            while data[e:e+2] != b"\0\0": e += 2
            out[tid] = data[so:e].decode("utf-16le")
    return out

def fmgs_of(msgbnd):
    """{nombre_fmg: {id: texto}} de un .msgbnd."""
    res = {}
    for fid, name, d in bnd4(msgbnd):
        base = name.replace("\\", "/").split("/")[-1].removesuffix(".fmg")
        try:
            res[base] = fmg(d)
        except Exception as ex:
            res[base] = {"__error__": str(ex)}
    return res
