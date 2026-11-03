#!/usr/bin/env python3
"""Convierte recursos/icono_pergamino.png (128x128 RGBA) en build/gen/icono_pergamino.h."""
from PIL import Image
ic = Image.open("recursos/icono_pergamino.png").convert("RGBA").resize((128, 128))
open("build/gen/icono_pergamino.h", "w").write(
    "static const unsigned char icono_pergamino[128*128*4] = {" + ",".join(str(v) for v in ic.tobytes()) + "};\n")
