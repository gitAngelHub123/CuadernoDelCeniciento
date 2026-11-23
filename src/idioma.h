// Cuaderno DS3 — idioma de los textos (es / en / zh). © 2026 Angel. MIT.
#pragma once
#include <string>
#include <cstdarg>

namespace idioma {
bool espanol();                                  // ¿se muestran los textos originales en español?
bool chino();                                    // ¿chino simplificado? (necesita la fuente CJK)
void iniciar(const std::string& valor, const std::string& carpetaJuego);   // valor de config: auto | es | en | zh
const char* T(const char* es);                   // traduce un literal (o un nombre de las tablas); si no hay traducción, devuelve el original
std::string fmt(const char* es, ...);            // T(es) como formato de printf
}
using idioma::T;
