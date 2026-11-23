// Cuaderno DS3 — idioma de los textos. © 2026 Angel. MIT.
#include "idioma.h"
#include "ingles_gen.h"
#include "ingles_texto.h"
#include "chino_gen.h"
#include "chino_texto.h"
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <windows.h>

void logf(const char* fmt, ...);   // main.cpp

namespace idioma {
namespace {
std::string actual = "es";
std::unordered_map<std::string, const char*> mapa;

// Idioma con el que Steam lanza el juego: "language" de UserConfig en appmanifest_374320.acf
// (carpeta del juego = steamapps\common\DARK SOULS III\Game).
std::string idiomaDeSteam(const std::string& carpetaJuego) {
    std::string ruta = carpetaJuego + "\\..\\..\\..\\appmanifest_374320.acf";
    FILE* f = fopen(ruta.c_str(), "r");
    if (!f) return "";
    char l[256]; std::string res; bool enUser = false;
    while (fgets(l, sizeof l, f)) {
        if (strstr(l, "\"UserConfig\"")) enUser = true;
        else if (enUser && strstr(l, "\"language\"")) {
            const char* q = strrchr(l, '"');          // cierre del valor
            if (q) { std::string s(l, (size_t)(q - l)); size_t a = s.rfind('"'); if (a != std::string::npos) res = s.substr(a + 1); }
            break;
        }
    }
    fclose(f);
    return res;
}

template <size_t N> void cargar(const ParIdioma (&t)[N]) { for (auto& p : t) mapa[p.es] = p.tr; }
}

bool espanol() { return actual == "es"; }
bool chino() { return actual == "zh"; }

void iniciar(const std::string& valor, const std::string& carpetaJuego) {
    std::string v = valor, origen = "config";
    if (v != "es" && v != "en" && v != "zh") {
        std::string st = idiomaDeSteam(carpetaJuego);
        origen = "Steam (" + st + ")";
        if (st == "spanish" || st == "latam") v = "es";
        else if (st == "schinese") v = "zh";
        else if (!st.empty()) v = "en";
        else {
            origen = "sistema";
            LANGID l = GetUserDefaultUILanguage();
            v = PRIMARYLANGID(l) == LANG_SPANISH ? "es"
              : l == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) || l == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE) ? "zh"
              : "en";
        }
    }
    actual = v;
    mapa.clear();
    // las frases propias mandan sobre los nombres del juego
    if (v == "en") { cargar(INGLES_GEN); cargar(INGLES_TEXTO); }
    if (v == "zh") { cargar(CHINO_GEN); cargar(CHINO_TEXTO); }
    logf("Idioma del cuaderno: %s (%s)", v == "es" ? "español" : v == "zh" ? "chino simplificado" : "inglés", origen.c_str());
}

const char* T(const char* es) {
    if (actual == "es" || !es) return es;
    auto it = mapa.find(es);
    return it == mapa.end() ? es : it->second;
}

std::string fmt(const char* es, ...) {
    char b[768];
    va_list a; va_start(a, es);
    vsnprintf(b, sizeof b, T(es), a);
    va_end(a);
    return b;
}
}
