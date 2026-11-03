// Cuaderno DS3 — búsqueda de patrones y lectura de estado del juego.
// © 2026 Angel. Patrones AOB tomados de SoulSplitter y The Grand Archives (comprobados en 1.15.2).
#include "juego.h"
#include <vector>
#include <cstring>
#include <set>

namespace {
struct Patron { std::vector<int> bytes; };

Patron compilar(const char* s) {
    Patron p;
    while (*s) {
        while (*s == ' ') ++s;
        if (!*s) break;
        if (*s == '?') { p.bytes.push_back(-1); while (*s == '?') ++s; }
        else { p.bytes.push_back((int)strtoul(s, (char**)&s, 16)); }
    }
    return p;
}

// Busca en las secciones ejecutables del módulo principal; devuelve la primera coincidencia.
uintptr_t buscar(uintptr_t base, const char* aob) {
    Patron p = compilar(aob);
    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);
    const size_t n = p.bytes.size();
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        auto* ini = (const uint8_t*)(base + sec->VirtualAddress);
        size_t tam = sec->Misc.VirtualSize;
        if (tam < n) continue;
        const uint8_t primero = (uint8_t)p.bytes[0];
        for (size_t j = 0; j + n <= tam; ++j) {
            if (ini[j] != primero) continue;
            size_t k = 1;
            for (; k < n; ++k) if (p.bytes[k] >= 0 && ini[j + k] != (uint8_t)p.bytes[k]) break;
            if (k == n) return (uintptr_t)(ini + j);
        }
    }
    return 0;
}

// Resuelve una instrucción con desplazamiento relativo a RIP (disp32 en +relOff, instrucción de insLen bytes).
uintptr_t resolver(uintptr_t base, const char* nombre, const char* aob, int relOff, int insLen, uintptr_t rvaEsperada) {
    uintptr_t hit = buscar(base, aob);
    if (!hit) { logf("Patrón %s: NO encontrado", nombre); return 0; }
    int32_t disp = 0; memcpy(&disp, (void*)(hit + relOff), 4);
    uintptr_t dir = hit + insLen + disp;
    logf("Patrón %s: rva 0x%llx%s", nombre, (unsigned long long)(dir - base),
         (rvaEsperada && dir - base != rvaEsperada) ? " (DISTINTA a la esperada en 1.15.2)" : "");
    return dir;
}

// Bloques de mapa -> índice de bloque de flags (de la tabla TGA, constante en 1.15.2).
int bloqueDe(int area, int d10000) {
    struct B { int a, d, b; };
    static const B t[] = {{30,0,3},{30,1,4},{31,0,5},{32,0,7},{33,0,9},{34,1,11},{35,0,12},{37,0,15},
                          {38,0,16},{39,0,17},{40,0,18},{41,0,19},{45,0,20},{50,0,23},{51,0,24},{51,1,25}};
    for (auto& x : t) if (x.a == area && x.d == d10000) return x.b;
    return -1;
}
} // namespace

bool Juego::iniciar() {
    base = (uintptr_t)GetModuleHandleA(nullptr);
    sGameDataMan  = resolver(base, "GameDataMan", "48 8B 05 ?? ?? ?? ?? 48 85 C0 ?? ?? 48 8B 40 ?? C3", 3, 7, 0x47572b8);
    sGameMan      = resolver(base, "GameMan", "48 8B ?? ?? ?? ?? 04 89 48 28 C3", 3, 7, 0x475ac00);
    sFieldArea    = resolver(base, "FieldArea", "48 8B 0D ?? ?? ?? ?? 48 85 C9 74 26 44 8B", 3, 7, 0x475abd0);
    sEventFlagMan = resolver(base, "SprjEventFlagMan", "48 8B 0D ?? ?? ?? ?? 44 0F B6 CB 41 B8 07 00 00 00 8B D6", 3, 7, 0x4752f68);
    sWorldChrMan  = resolver(base, "WorldChrMan", "48 8B 1D ?? ?? ?? 04 48 8B F9 48 85 DB ?? ?? 8B 11 85 D2 ?? ?? 8D", 3, 7, 0x477fdb8);
    // función general de textos: se llama dentro de "nombre de arma" (getter-0x1D, call en +0x18)
    if (uintptr_t hit = buscar(base, "48 85 C0 75 52 44 8D 40 0B")) {
        uintptr_t llamada = hit - 0x1D + 0x18;
        if (*(uint8_t*)llamada == 0xE8) {
            int32_t rel; memcpy(&rel, (void*)(llamada + 1), 4);
            fObtenerMensaje = llamada + 5 + rel;
        }
    }
    logf("Función de textos: %s rva 0x%llx", fObtenerMensaje ? "ok" : "NO encontrada",
         (unsigned long long)(fObtenerMensaje ? fObtenerMensaje - base : 0));
    listo = sGameDataMan && sGameMan && sEventFlagMan && sWorldChrMan;
    logf("Lectura del juego: %s", listo ? "lista" : "INCOMPLETA");
    return listo;
}

bool Juego::idAOffset(uint32_t id, uint32_t& off, uint8_t& bit) {
    int chunk;
    if (id < 10000) chunk = id / 1000;
    else {
        int bl = bloqueDe((id / 100000) % 100, (id / 10000) % 10);
        if (bl < 0 || id / 10000000 != 1) return false;
        chunk = bl * 10 + (id / 1000) % 10;
    }
    uint32_t b = id % 1000;
    off = chunk * 0x80 + (b >> 5) * 4 + (3 - ((b & 31) >> 3));
    bit = 7 - (b & 7);
    return true;
}

bool Juego::flagBruto(uint32_t off, uint8_t bit, bool& valor) const {
    if (demo) { valor = demoBrutos.count(off * 8 + bit) > 0; return true; }
    if (!listo || bit > 7) return false;
    uintptr_t arr = leerPtr(leerPtr(sEventFlagMan));
    uint8_t v;
    if (!arr || !leer(arr + off, v)) return false;
    valor = (v >> bit) & 1;
    return true;
}

bool Juego::flag(uint32_t id) const {
    if (demo) return demoFlags.count(id) > 0;
    uint32_t off; uint8_t bit; bool v = false;
    if (!idAOffset(id, off, bit)) return false;
    return flagBruto(off, bit, v) && v;
}

bool Juego::bloqueFlags(uint32_t desde, uint32_t hasta, uint8_t* out) const {
    if (demo) return false;
    uintptr_t arr = leerPtr(leerPtr(sEventFlagMan));
    if (!arr) return false;
    SIZE_T n = 0;
    return ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(arr + desde), out, hasta - desde, &n) && n == hasta - desde;
}

bool Juego::jugadorCargado() const {
    if (demo) return true;
    return listo && leerPtr(leerPtr(sWorldChrMan) + 0x80) != 0;
}

int Juego::regionActual() const {
    if (demo) return demoRegion;
    uintptr_t pl = leerPtr(leerPtr(sWorldChrMan) + 0x80);
    int r = -1;
    return (pl && leer(pl + 0x1ABC, r)) ? r : -1;
}

int Juego::enLinea() const {
    if (demo) return 0;
    uintptr_t gm = leerPtr(sGameMan);
    uint8_t v;
    return (gm && leer(gm + 0xB1C, v)) ? (v ? 1 : 0) : -1;
}

int Juego::dialogoActual() const {
    if (demo) return -1;
    uintptr_t pl = leerPtr(leerPtr(sWorldChrMan) + 0x80);
    uintptr_t a = leerPtr(leerPtr(pl + 0x1F90) + 0x8);
    int t = -1;
    return (pl && a && leer(a + 0x94, t)) ? t : -1;
}

std::wstring Juego::nombrePersonaje() const {
    if (demo) return L"Demostración";
    uintptr_t pgd = leerPtr(leerPtr(sGameDataMan) + 0x10);
    wchar_t buf[17] = {};
    if (!pgd) return L"";
    for (int i = 0; i < 16; ++i) if (!leer(pgd + 0x88 + i * 2, buf[i]) || !buf[i]) break;
    return buf;
}

// Inventario: [[GameDataMan]+10]+3D0 = EquipInventoryData; lista en +18 (entradas de 16 bytes: +4 id, +8 cantidad),
// número en +20 (TGA). Se lee con margen porque los objetos clave pueden ir después de los normales.
bool Juego::inventario(std::set<uint32_t>& ids) const {
    if (demo) return false;
    uintptr_t pgd = leerPtr(leerPtr(sGameDataMan) + 0x10);
    if (!pgd) return false;
    uintptr_t lista = leerPtr(pgd + 0x3D0 + 0x18);
    int n = 0;
    if (!lista || !leer(pgd + 0x3D0 + 0x20, n) || n < 0 || n > 4096) return false;
    // los objetos clave van en una zona posterior de la misma lista: leerla siempre entera
    int total = n + 1024; if (total < 2560) total = 2560; if (total > 4096) total = 4096;
    std::vector<uint8_t> buf(total * 16);
    SIZE_T leido = 0;
    ReadProcessMemory(GetCurrentProcess(), (LPCVOID)lista, buf.data(), buf.size(), &leido);
    for (size_t i = 0; i + 16 <= leido; i += 16) {
        uint32_t id, cant;
        memcpy(&id, &buf[i + 4], 4); memcpy(&cant, &buf[i + 8], 4);
        if (id != 0xFFFFFFFF && id != 0 && cant >= 1 && cant <= 9999) ids.insert(id);
    }
    return true;
}

// Ranura de guardado activa: [GameMan]+A60 (TGA "Save Slot (Profile Index)").
int Juego::ranura() const {
    if (demo) return 0;
    uintptr_t gm = leerPtr(sGameMan);
    int r = -1;
    return (gm && leer(gm + 0xA60, r) && r >= 0 && r < 100) ? r : -1;
}
