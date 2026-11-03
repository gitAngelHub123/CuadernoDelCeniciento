// Cuaderno DS3 — lectura segura de la memoria del juego (solo lectura, nunca escribe).
// © 2026 Angel.
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>
#include <set>
#include "datos.h"

void logf(const char* fmt, ...);

template <class T> static bool leer(uintptr_t dir, T& out) {
    if (dir < 0x10000) return false;
    SIZE_T n = 0;
    return ReadProcessMemory(GetCurrentProcess(), (LPCVOID)dir, &out, sizeof(T), &n) && n == sizeof(T);
}
static uintptr_t leerPtr(uintptr_t dir) { uintptr_t v = 0; return leer(dir, v) ? v : 0; }

struct Juego {
    uintptr_t base = 0;
    // direcciones de las variables globales (estáticas) del ejecutable
    uintptr_t fObtenerMensaje = 0;   // MsgRepository: texto(repo, ?, categoría, id)
    uintptr_t sGameDataMan = 0, sGameMan = 0, sEventFlagMan = 0, sWorldChrMan = 0, sFieldArea = 0;
    bool listo = false;
    // modo demostración (config demo=1): datos de ejemplo para probar la interfaz sin el juego
    bool demo = false;
    std::set<uint32_t> demoFlags;     // IDs de flags activos
    std::set<uint32_t> demoBrutos;    // off*8+bit activos
    int demoRegion = 310002;

    bool iniciar();

    // flags
    bool flagBruto(uint32_t off, uint8_t bit, bool& valor) const;
    bool flag(uint32_t id) const;                        // false si no se puede leer
    static bool idAOffset(uint32_t id, uint32_t& off, uint8_t& bit);
    bool bloqueFlags(uint32_t desde, uint32_t hasta, uint8_t* out) const;

    // estado
    bool jugadorCargado() const;
    int  regionActual() const;     // playRegionId, -1 si no hay
    int  enLinea() const;          // 1 en línea, 0 sin conexión, -1 desconocido
    int  dialogoActual() const;    // TalkID, -1 si no hay conversación
    std::wstring nombrePersonaje() const;
    int ranura() const;   // ranura de guardado (perfil) activa, -1 si no se puede leer
    bool inventario(std::set<uint32_t>& ids) const;   // IDs completos (tipo en los 4 bits altos)
};
