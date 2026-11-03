#!/bin/bash
# Ejecuta el modo demostración con Proton y guarda captura PNG.
# Uso: probar_demo.sh <vista> <salida.png> ["demo_paso=5\ndemo_captura=30\n"]
set -e
SP=${TMPDIR:-/tmp}/cuaderno_demo
T=$SP/prueba_ds3; P=$HOME/Proyectos/CuadernoDS3
mkdir -p $T/CuadernoDS3 $SP/prefijo_prueba
cp $P/build/dinput8.dll $P/prueba/prueba.exe $T/
printf 'demo=1\ndemo_vista=%s\ndiagnostico=0\n%b' "$1" "${3:-}" > $T/CuadernoDS3/config.ini
rm -f $T/CuadernoDS3/captura_demo.bmp $T/CuadernoDS3.log
STEAM_COMPAT_CLIENT_INSTALL_PATH=$HOME/.local/share/Steam STEAM_COMPAT_DATA_PATH=$SP/prefijo_prueba \
  timeout 200 "$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS Latest/proton" run "$T/prueba.exe" >/dev/null 2>&1
python3 -I - "$T/CuadernoDS3/captura_demo.bmp" "$2" <<'PY'
import struct,zlib,sys
d=open(sys.argv[1],"rb").read()
off=struct.unpack_from("<I",d,10)[0]; W,H=struct.unpack_from("<ii",d,18); fila=(W*3+3)&~3
x0=0; rows=[]
for y in range(H):
    src=d[off+(H-1-y)*fila:off+(H-1-y)*fila+W*3]; r=bytearray(b"\0")
    for x in range(x0,W): r+=bytes((src[x*3+2],src[x*3+1],src[x*3]))
    rows.append(bytes(r))
c=lambda t,b: struct.pack(">I",len(b))+t+b+struct.pack(">I",zlib.crc32(t+b)&0xffffffff)
open(sys.argv[2],"wb").write(b"\x89PNG\r\n\x1a\n"+c(b"IHDR",struct.pack(">IIBBBBB",W-x0,H,8,2,0,0,0))+c(b"IDAT",zlib.compress(b"".join(rows),6))+c(b"IEND",b""))
PY
grep -v '^$' $T/CuadernoDS3.log | tail -3
