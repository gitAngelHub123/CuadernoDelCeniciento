// Cuaderno DS3 — idioma de los textos. © 2026 Angel. MIT.
#include "idioma.h"
#include "ingles_gen.h"
#include "ingles_texto.h"
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <windows.h>

void logf(const char* fmt, ...);   // main.cpp

namespace idioma {
namespace {
bool en = false;
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
}

bool ingles() { return en; }

void iniciar(const std::string& valor, const std::string& carpetaJuego) {
    std::string v = valor, origen = "config";
    if (v != "es" && v != "en") {
        std::string st = idiomaDeSteam(carpetaJuego);
        origen = "Steam (" + st + ")";
        if (st == "spanish" || st == "latam") v = "es";
        else if (!st.empty()) v = "en";
        else {
            origen = "sistema";
            v = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_SPANISH ? "es" : "en";
        }
    }
    en = v == "en";
    mapa.clear();
    if (en) {
        for (auto& p : INGLES_GEN) mapa[p.es] = p.en;
        for (auto& p : INGLES_TEXTO) mapa[p.es] = p.en;     // las frases propias mandan sobre los nombres del juego
    }
    logf("Idioma del cuaderno: %s (%s)", en ? "inglés" : "español", origen.c_str());
}

const char* T(const char* es) {
    if (!en || !es) return es;
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
