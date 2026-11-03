// Cuaderno DS3 — sonido de página sintetizado (sin archivos externos).  © 2026 Angel.
#pragma once
#include <cstdint>
#include <vector>

namespace sonido {
// Genera un WAV completo (cabecera RIFF + PCM 16 bits mono 44,1 kHz) de una página pasando.
// variante cambia los detalles; apertura=true hace un barrido algo más largo y grave.
std::vector<uint8_t> paginaWav(int variante, bool apertura, float volumen);
}
