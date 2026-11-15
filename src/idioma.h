// Cuaderno DS3 — idioma de los textos (es / en). © 2026 Angel. MIT.
#pragma once
#include <string>
#include <cstdarg>

namespace idioma {
bool ingles();                                   // ¿se muestran los textos en inglés?
void iniciar(const std::string& valor, const std::string& carpetaJuego);   // valor de config: auto | es | en
const char* T(const char* es);                   // traduce un literal (o un nombre de las tablas); si no hay traducción, devuelve el original
std::string fmt(const char* es, ...);            // T(es) como formato de printf
}
using idioma::T;
