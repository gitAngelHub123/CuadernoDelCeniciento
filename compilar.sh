#!/bin/bash
# Compila dinput8.dll (Cuaderno DS3) para Windows x64 con MinGW.
set -e
cd "$(dirname "$0")"
I=terceros/imgui; M=terceros/minhook
mkdir -p build/obj build/gen
[ build/gen/icono_pergamino.h -nt recursos/icono_pergamino.png ] || herramientas/.venv/bin/python -I herramientas/generar_icono.py
# fuentes embebidas (IM FELL English, SIL OFL 1.1)
[ build/gen/fuente_regular.h -nt recursos/IMFeENrm28P.ttf ] || (cd recursos && xxd -i -n fuente_regular IMFeENrm28P.ttf > ../build/gen/fuente_regular.h)
[ build/gen/fuente_cursiva.h -nt recursos/IMFeENit28P.ttf ] || (cd recursos && xxd -i -n fuente_cursiva IMFeENit28P.ttf > ../build/gen/fuente_cursiva.h)
for f in buffer hook trampoline hde/hde64; do
  x86_64-w64-mingw32-gcc -O2 -c $M/src/$f.c -I$M/include -o build/obj/$(basename $f).o
done
x86_64-w64-mingw32-g++ -std=c++17 -O2 -shared -o build/dinput8.dll \
  src/main.cpp src/juego.cpp src/cuaderno.cpp src/sonido.cpp \
  $I/imgui.cpp $I/imgui_draw.cpp $I/imgui_tables.cpp $I/imgui_widgets.cpp \
  $I/backends/imgui_impl_win32.cpp $I/backends/imgui_impl_dx11.cpp \
  build/obj/*.o \
  -Isrc -Ibuild/gen -I$I -I$I/backends -I$M/include \
  -static -static-libgcc -static-libstdc++ -s \
  -ld3d11 -lwinmm -ldxgi -ld3dcompiler -ldwmapi -lgdi32
echo "OK: build/dinput8.dll"
