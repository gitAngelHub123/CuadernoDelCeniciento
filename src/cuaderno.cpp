// Cuaderno DS3 — lógica e interfaz: zona, ruta principal, zonas descubiertas, NPCs, fotos, controles.
// © 2026 Angel. Usa Dear ImGui (MIT). Fuente: IM FELL English (SIL OFL 1.1, Igino Marini).
#include "cuaderno.h"
#include "imgui.h"
#include "MinHook.h"
#include <xinput.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "fuente_regular.h"
#include "fuente_cursiva.h"
#include "icono_pergamino.h"
#include "sonido.h"
#include "hogueras.h"
#include "misiones_gen.h"
#include <mmsystem.h>

namespace {
// ---------------------------------------------------------------- estado general
Juego g;
ID3D11Device* dev = nullptr;
ID3D11DeviceContext* ctx = nullptr;
HWND hwnd = nullptr;
std::string carpetaJuego, carpetaMod;
ImFont *fHud = nullptr, *fTitulo = nullptr, *fTexto = nullptr, *fPeque = nullptr, *fCursiva = nullptr;
ID3D11ShaderResourceView* texPapel = nullptr;
ID3D11ShaderResourceView* texIcono = nullptr;

// config
int teclaCuaderno = 'K', teclaSiguiente = 'L', teclaAnterior = 'J', teclaFoto = 'P';
WORD mandoAbrir = XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB, mandoSiguiente = XINPUT_GAMEPAD_RIGHT_THUMB, mandoAnterior = XINPUT_GAMEPAD_LEFT_THUMB;
float escala = 1.0f;
float volumen = 0.7f;
bool mostrarIcono = true, diagnostico = true, demo = false;
int demoAccion = 1, demoVista = 1, demoPaso = -1, demoCuadro = 0, demoCaptura = 40;

// vista: 0 cerrado, 1 zonas, 2.. páginas de NPCs
int vista = 0;
volatile bool modoEnLinea = false;   // juego conectado: el mod no hace nada

// partida actual
std::wstring personaje;
std::wstring carpetaPartida;
std::set<int> npcsConocidos, zonasVisitadas;
std::set<uint32_t> objetosVistos, objetosAhora;
std::map<int, ID3D11ShaderResourceView*> fotos;
bool estadoSucio = false;

// foto pendiente
int fotoPendienteNpc = -1;
ULONGLONG fotoCuando = 0;
int ultimoDialogoNpc = -1;

// diagnóstico
uint8_t diagPrevio[DIAG_HASTA - DIAG_DESDE] = {};
bool diagIniciado = false;
int regionPrevia = -2, dialogoPrevio = -2, enLineaPrevio = -2;
ULONGLONG diagUltimo = 0;

std::string utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

// ---------------------------------------------------------------- configuración
int teclaDeTexto(const char* v) {
    if (strlen(v) == 1) return toupper(v[0]);
    if ((v[0] == 'F' || v[0] == 'f') && atoi(v + 1) >= 1 && atoi(v + 1) <= 12) return VK_F1 + atoi(v + 1) - 1;
    return 0;
}
WORD botonesDeTexto(const char* v) {
    static const struct { const char* n; WORD b; } t[] = {
        {"L3", XINPUT_GAMEPAD_LEFT_THUMB}, {"R3", XINPUT_GAMEPAD_RIGHT_THUMB}, {"LB", XINPUT_GAMEPAD_LEFT_SHOULDER},
        {"RB", XINPUT_GAMEPAD_RIGHT_SHOULDER}, {"A", XINPUT_GAMEPAD_A}, {"B", XINPUT_GAMEPAD_B}, {"X", XINPUT_GAMEPAD_X},
        {"Y", XINPUT_GAMEPAD_Y}, {"SELECT", XINPUT_GAMEPAD_BACK}, {"START", XINPUT_GAMEPAD_START},
        {"ARRIBA", XINPUT_GAMEPAD_DPAD_UP}, {"ABAJO", XINPUT_GAMEPAD_DPAD_DOWN},
        {"IZQUIERDA", XINPUT_GAMEPAD_DPAD_LEFT}, {"DERECHA", XINPUT_GAMEPAD_DPAD_RIGHT}};
    WORD r = 0; char buf[128]; strncpy(buf, v, 127); buf[127] = 0;
    for (char* tok = strtok(buf, "+ "); tok; tok = strtok(nullptr, "+ ")) {
        for (char* c = tok; *c; ++c) *c = toupper(*c);
        for (auto& x : t) if (!strcmp(tok, x.n)) r |= x.b;
    }
    return r;
}

void cargarConfig() {
    std::string ruta = carpetaMod + "\\config.ini";
    FILE* f = fopen(ruta.c_str(), "r");
    if (!f) {
        f = fopen(ruta.c_str(), "w");
        if (f) {
            fputs("; Cuaderno DS3 - configuración\n"
                  "; Teclas: una letra (J) o F1..F12. Mando: botones unidos con + (L3+R3, SELECT+ARRIBA, LB+Y...)\n"
                  "; K abre/cierra, L página siguiente, J página anterior, P repite el retrato (en una conversación)\n"
                  "tecla_cuaderno=K\n"
                  "tecla_siguiente=L\n"
                  "tecla_anterior=J\n"
                  "tecla_retrato=P\n"
                  "; Mando: L3+R3 abre/cierra, R3 página siguiente, L3 página anterior\n"
                  "mando_abrir=L3+R3\n"
                  "mando_siguiente=R3\n"
                  "mando_anterior=L3\n"
                  "escala=1.0\n"
                  "; volumen del sonido de página: 0 (sin sonido) a 1\n"
                  "volumen=0.7\n"
                  "mostrar_icono=1\n"
                  "diagnostico=1\n", f);
            fclose(f);
        }
        logf("config.ini creado con valores por defecto");
        return;
    }
    char linea[256];
    bool nuevas = false;
    while (fgets(linea, sizeof linea, f)) {
        if (linea[0] == ';' || linea[0] == '#') continue;
        char* eq = strchr(linea, '='); if (!eq) continue;
        *eq = 0; char* v = eq + 1; v[strcspn(v, "\r\n")] = 0;
        std::string k = linea;
        if (k == "tecla_cuaderno") { if (int t = teclaDeTexto(v)) teclaCuaderno = t; nuevas = true; }
        else if (k == "tecla_siguiente") { if (int t = teclaDeTexto(v)) teclaSiguiente = t; }
        else if (k == "tecla_anterior") { if (int t = teclaDeTexto(v)) teclaAnterior = t; }
        else if (k == "tecla_retrato") { if (int t = teclaDeTexto(v)) teclaFoto = t; }
        else if (k == "mando_abrir") { if (WORD b = botonesDeTexto(v)) mandoAbrir = b; }
        else if (k == "mando_siguiente") { if (WORD b = botonesDeTexto(v)) mandoSiguiente = b; }
        else if (k == "mando_anterior") { if (WORD b = botonesDeTexto(v)) mandoAnterior = b; }
        else if (k == "escala") { float e = (float)atof(v); if (e >= 0.5f && e <= 3.f) escala = e; }
        else if (k == "volumen") { float e = (float)atof(v); if (e >= 0.f && e <= 1.f) volumen = e; }
        else if (k == "mostrar_icono") mostrarIcono = atoi(v) != 0;
        else if (k == "diagnostico") diagnostico = atoi(v) != 0;
        else if (k == "demo") demo = atoi(v) != 0;
        else if (k == "demo_vista") demoVista = atoi(v);
        else if (k == "demo_paso") demoPaso = atoi(v);       // cuadro en que se "pulsa" J
        else if (k == "demo_captura") demoCaptura = atoi(v); // cuadro de la captura
        else if (k == "demo_accion") demoAccion = atoi(v);   // 0 cuaderno, 1 siguiente, 2 anterior
    }
    fclose(f);
    if (!nuevas && !demo) {
        // config de una versión anterior (tecla_abrir/tecla_foto): añadir los controles nuevos
        if (FILE* a = fopen(ruta.c_str(), "a")) {
            fputs("\n; Controles nuevos (las líneas tecla_abrir y tecla_foto ya no se usan)\n"
                  "; K abre/cierra, L página siguiente, J página anterior, P repite el retrato (en una conversación)\n"
                  "tecla_cuaderno=K\ntecla_siguiente=L\ntecla_anterior=J\ntecla_retrato=P\n", a);
            fclose(a);
            logf("config.ini actualizado con los controles nuevos");
        }
    }
    logf("config: cuaderno=%c siguiente=%c anterior=%c retrato=%c mando=0x%04x escala=%.2f",
         teclaCuaderno, teclaSiguiente, teclaAnterior, teclaFoto, mandoAbrir, escala);
}

// ---------------------------------------------------------------- textura de papel
float ruido(int x, int y, int s) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263 + s * 2147483647);
    h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
    return (h & 0xFFFF) / 65535.f;
}
float ruidoSuave(float x, float y, int s) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = ruido(xi, yi, s), b = ruido(xi + 1, yi, s), c = ruido(xi, yi + 1, s), d = ruido(xi + 1, yi + 1, s);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

ID3D11ShaderResourceView* crearTextura(int w, int h, const uint32_t* rgba) {
    D3D11_TEXTURE2D_DESC d{};
    d.Width = w; d.Height = h; d.MipLevels = 1; d.ArraySize = 1;
    d.Format = DXGI_FORMAT_R8G8B8A8_UNORM; d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DEFAULT; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{rgba, (UINT)w * 4, 0};
    ID3D11Texture2D* t = nullptr; ID3D11ShaderResourceView* v = nullptr;
    if (FAILED(dev->CreateTexture2D(&d, &sd, &t))) return nullptr;
    dev->CreateShaderResourceView(t, nullptr, &v);
    t->Release();
    return v;
}

void crearPapel() {
    const int N = 512;
    std::vector<uint32_t> px(N * N);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float u = x / (float)N, v = y / (float)N;
            float n = 0, amp = 0.5f, fr = 4;
            for (int o = 0; o < 5; ++o) { n += amp * ruidoSuave(u * fr, v * fr, o + 7); amp *= 0.5f; fr *= 2.1f; }
            float fibra = (ruidoSuave(u * 40, v * 9, 31) + ruidoSuave(u * 7, v * 55, 32) - 1.f) * 0.035f;
            float mancha = ruidoSuave(u * 2.2f, v * 2.2f, 99);
            mancha = mancha > 0.72f ? (mancha - 0.72f) * 1.6f : 0;
            float dx = fabsf(u - 0.5f) * 2, dy = fabsf(v - 0.5f) * 2;
            float borde = powf(fmaxf(dx, dy), 6.f) * 0.55f + (dx * dx + dy * dy) * 0.08f;
            float k = 1.f - (n - 0.5f) * 0.22f - fibra - mancha * 0.35f - borde;
            int r = (int)(232 * k), gr = (int)(214 * k), b = (int)(170 * k - mancha * 25);
            auto cl = [](int c) { return c < 0 ? 0 : c > 255 ? 255 : c; };
            px[y * N + x] = 0xFF000000u | (cl(b) << 16) | (cl(gr) << 8) | cl(r);
        }
    texPapel = crearTextura(N, N, px.data());
}

// ---------------------------------------------------------------- partida (persistencia)
void guardarEstado() {
    if (carpetaPartida.empty() || !estadoSucio) return;
    FILE* f = _wfopen((carpetaPartida + L"\\estado.txt").c_str(), L"w");
    if (!f) return;
    for (int n : npcsConocidos) fprintf(f, "npc %d\n", n);
    for (int z : zonasVisitadas) fprintf(f, "zona %d\n", z);
    for (uint32_t o : objetosVistos) fprintf(f, "obj %u\n", o);
    fclose(f);
    estadoSucio = false;
}

ID3D11ShaderResourceView* cargarBmp(const std::wstring& ruta) {
    FILE* f = _wfopen(ruta.c_str(), L"rb");
    if (!f) return nullptr;
    BITMAPFILEHEADER fh; BITMAPINFOHEADER ih;
    bool ok = fread(&fh, sizeof fh, 1, f) == 1 && fread(&ih, sizeof ih, 1, f) == 1 && ih.biBitCount == 24;
    std::vector<uint32_t> px;
    if (ok) {
        int w = ih.biWidth, h = abs(ih.biHeight), fila = (w * 3 + 3) & ~3;
        std::vector<uint8_t> raw(fila * h);
        fseek(f, fh.bfOffBits, SEEK_SET);
        ok = fread(raw.data(), 1, raw.size(), f) == raw.size();
        px.resize(w * h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                const uint8_t* p = &raw[(ih.biHeight > 0 ? h - 1 - y : y) * fila + x * 3];
                px[y * w + x] = 0xFF000000u | (p[0] << 16) | (p[1] << 8) | p[2];
            }
        fclose(f);
        return ok ? crearTextura(w, h, px.data()) : nullptr;
    }
    fclose(f);
    return nullptr;
}

void cambiarPartida(const std::wstring& nombre, int ranura) {
    guardarEstado();
    for (auto& [k, v] : fotos) if (v) v->Release();
    fotos.clear(); npcsConocidos.clear(); zonasVisitadas.clear(); objetosVistos.clear(); objetosAhora.clear();
    personaje = nombre;
    std::wstring limpio;
    for (wchar_t c : nombre) limpio += (wcschr(L"\\/:*?\"<>|", c) || c < 32) ? L'_' : c;
    if (limpio.empty()) limpio = L"sin_nombre";
    std::wstring base = std::wstring(carpetaMod.begin(), carpetaMod.end()) + L"\\partidas";
    CreateDirectoryW(base.c_str(), nullptr);
    // carpeta por personaje y ranura; la carpeta antigua (solo nombre) se adopta la primera vez
    std::wstring dir = base + L"\\" + limpio + (ranura >= 0 ? L" - ranura " + std::to_wstring(ranura + 1) : L"");
    std::wstring antigua = base + L"\\" + limpio;
    if (ranura >= 0 && GetFileAttributesW(dir.c_str()) == INVALID_FILE_ATTRIBUTES && GetFileAttributesW(antigua.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (MoveFileW(antigua.c_str(), dir.c_str())) logf("Partida: carpeta antigua de %s pasa a la ranura %d", utf8(nombre).c_str(), ranura + 1);
    }
    CreateDirectoryW(dir.c_str(), nullptr);
    CreateDirectoryW((dir + L"\\fotos").c_str(), nullptr);
    carpetaPartida = dir;
    if (FILE* f = _wfopen((carpetaPartida + L"\\estado.txt").c_str(), L"r")) {
        char tipo[16]; long long v;
        while (fscanf(f, "%15s %lld", tipo, &v) == 2) {
            if (!strcmp(tipo, "npc")) npcsConocidos.insert((int)v);
            else if (!strcmp(tipo, "zona")) zonasVisitadas.insert((int)v);
            else if (!strcmp(tipo, "obj")) objetosVistos.insert((uint32_t)v);
        }
        fclose(f);
    }
    for (int i = 0; i < N_TOTAL; ++i) {
        wchar_t ruta[MAX_PATH]; swprintf(ruta, MAX_PATH, L"%ls\\fotos\\npc%02d.bmp", carpetaPartida.c_str(), i);
        if (auto* t = cargarBmp(ruta)) fotos[i] = t;
    }
    logf("Partida: %s, ranura %d (%d NPCs conocidos, %d zonas visitadas, %d fotos)", utf8(nombre).c_str(), ranura + 1,
         (int)npcsConocidos.size(), (int)zonasVisitadas.size(), (int)fotos.size());
}

// ---------------------------------------------------------------- foto en sepia
// Recorte cuadrado -> retrato sepia de S×S (niveles automáticos, viñeta y grano)
constexpr int S_FOTO = 224;
std::vector<uint32_t> crearRetrato(const uint8_t* datos, UINT pitch, bool bgra, int W, int H, int x0, int y0, int lado) {
    const int S = S_FOTO;
    auto px = [&](int x, int y) { return datos + y * pitch + x * 4; };
    int hist[256] = {}, total = 0;
    for (int y = 0; y < S; y += 2)
        for (int x = 0; x < S; x += 2) {
            int X = x0 + x * lado / S, Y = y0 + y * lado / S;
            if (X < 0 || Y < 0 || X >= W || Y >= H) continue;
            const uint8_t* p = px(X, Y);
            hist[(int)(0.299f * (bgra ? p[2] : p[0]) + 0.587f * p[1] + 0.114f * (bgra ? p[0] : p[2]))]++; ++total;
        }
    int lo = 0, hi = 255, acc = 0;
    for (int i = 0; i < 256; ++i) { acc += hist[i]; if (acc > total * 0.02f) { lo = i; break; } }
    acc = 0;
    for (int i = 255; i >= 0; --i) { acc += hist[i]; if (acc > total * 0.005f) { hi = i; break; } }
    if (hi - lo < 70) hi = lo + 70;   // no estirar de más las escenas oscuras
    std::vector<uint32_t> out(S * S);
    for (int y = 0; y < S; ++y)
        for (int x = 0; x < S; ++x) {
            float r = 0, gr = 0, b = 0; int cnt = 0;
            for (int sy = 0; sy < 2; ++sy)
                for (int sx = 0; sx < 2; ++sx) {
                    int X = x0 + (x * 2 + sx) * lado / (S * 2), Y = y0 + (y * 2 + sy) * lado / (S * 2);
                    if (X < 0 || Y < 0 || X >= W || Y >= H) continue;
                    const uint8_t* p = px(X, Y);
                    r += bgra ? p[2] : p[0]; gr += p[1]; b += bgra ? p[0] : p[2]; ++cnt;
                }
            if (cnt) { r /= cnt; gr /= cnt; b /= cnt; }
            float lum = 0.299f * r + 0.587f * gr + 0.114f * b;
            lum = fminf(1.f, fmaxf(0.f, (lum - lo) / (float)(hi - lo)));
            lum = 12.f + 198.f * powf(lum, 1.1f);                    // más contenido: menos brillo
            float u = x / (float)S - 0.5f, v = y / (float)S - 0.5f;
            float vig = 1.f - (u * u + v * v) * 1.3f;
            float grano = (ruido(x, y, 5) - 0.5f) * 14.f;
            float l = fmaxf(0.f, lum * vig + grano);
            int R = (int)fminf(255.f, l * 1.07f + 18), G = (int)fminf(255.f, l * 0.91f + 12), B = (int)fminf(255.f, l * 0.68f + 4);
            out[y * S + x] = 0xFF000000u | (B << 16) | (G << 8) | R;
        }
    return out;
}

// Puntúa un encuadre: detalle (gradiente) ponderado al centro, penalizando oscuridad o saturación.
float puntuarEncuadre(const uint8_t* datos, UINT pitch, bool bgra, int W, int H, int x0, int y0, int lado) {
    auto lum = [&](int X, int Y) {
        X = X < 0 ? 0 : X >= W ? W - 1 : X; Y = Y < 0 ? 0 : Y >= H ? H - 1 : Y;
        const uint8_t* p = datos + Y * pitch + X * 4;
        return 0.299f * (bgra ? p[2] : p[0]) + 0.587f * p[1] + 0.114f * (bgra ? p[0] : p[2]);
    };
    const int N = 40; int paso = lado / N; if (paso < 2) paso = 2;
    float grad = 0, suma = 0, peso = 0;
    for (int j = 0; j < N; ++j)
        for (int i = 0; i < N; ++i) {
            int X = x0 + i * lado / N, Y = y0 + j * lado / N;
            float l = lum(X, Y);
            float g = fabsf(lum(X + paso, Y) - l) + fabsf(lum(X, Y + paso) - l);
            float u = i / (float)N - 0.5f, v = j / (float)N - 0.5f;
            float w = expf(-(u * u + v * v) * 6.f);
            grad += g * w; suma += l * w; peso += w;
        }
    grad /= peso; float media = suma / peso;
    float luz = media < 25 ? media / 25.f : media > 215 ? (255 - media) / 40.f : 1.f;
    return grad * fmaxf(0.f, luz);
}

// ---------------------------------------------------------------- sesión de fotos
// Oculta un instante al personaje del jugador, toma 10 fotos y se queda con el mejor encuadre.
struct SesionFoto {
    int npc = -1, cuadro = 0, tomas = 0;
    float mejor = -1; std::vector<uint32_t> retrato;
    float npcPos[3] = {0, 0, 0}; float npcDist = 0; bool npcOk = false;
    uintptr_t dirDibujo = 0; uint8_t original = 0; bool oculto = false;
} ses;
constexpr int FOTO_TOMAS = 10, FOTO_ESPERA = 6, FOTO_CADA = 2;

void ocultarJugador(bool ocultar) {
    if (ocultar) {
        uintptr_t pl = leerPtr(leerPtr(g.sWorldChrMan) + 0x80);
        if (!pl) return;
        ses.dirDibujo = pl + 0x1EE9;               // "Toggle Draw" (TGA): bit 6
        if (!leer(ses.dirDibujo, ses.original)) { ses.dirDibujo = 0; return; }
        uint8_t v = ses.original ^ 0x40; SIZE_T n;
        ses.oculto = WriteProcessMemory(GetCurrentProcess(), (LPVOID)ses.dirDibujo, &v, 1, &n) && n == 1;
        logf("Foto: ocultar jugador 0x%02x -> 0x%02x %s", ses.original, v, ses.oculto ? "ok" : "falló");
    } else if (ses.oculto) {
        uintptr_t pl = leerPtr(leerPtr(g.sWorldChrMan) + 0x80);
        if (pl && pl + 0x1EE9 == ses.dirDibujo) {   // mismo personaje: restaurar el bit original
            uint8_t v; SIZE_T n;
            if (leer(ses.dirDibujo, v)) { v = (v & ~0x40) | (ses.original & 0x40); WriteProcessMemory(GetCurrentProcess(), (LPVOID)ses.dirDibujo, &v, 1, &n); }
        }
        ses.oculto = false;
    }
}

// Ajuste "HUD" (TGA): opción guardada [[GameDataMan]+58]+11 y valor vivo [[GameDataMan]+60]+F91 (0 = no).
// (Los subtítulos NO se tocan: el usuario prefiere los retratos así.)
bool hudOculto = false; uint8_t hudOrigA = 1, hudOrigB = 1; uintptr_t hudA = 0, hudB = 0;
void escribirByte(uintptr_t dir, uint8_t v) { SIZE_T n; if (dir) WriteProcessMemory(GetCurrentProcess(), (LPVOID)dir, &v, 1, &n); }
void ocultarInterfaz(bool ocultar) {
    uintptr_t gdm = leerPtr(g.sGameDataMan);
    if (ocultar) {
        uintptr_t a = leerPtr(gdm + 0x58), b = leerPtr(gdm + 0x60);
        hudA = a ? a + 0x11 : 0; hudB = b ? b + 0xF91 : 0;
        if ((hudA && !leer(hudA, hudOrigA)) || (hudB && !leer(hudB, hudOrigB))) { hudA = hudB = 0; return; }
        escribirByte(hudA, 0); escribirByte(hudB, 0);
        hudOculto = hudA || hudB;
        logf("Foto: ocultar interfaz (%u, %u)", hudOrigA, hudOrigB);
    } else if (hudOculto) {
        if (hudA && leerPtr(gdm + 0x58) + 0x11 == hudA) escribirByte(hudA, hudOrigA);
        if (hudB && leerPtr(gdm + 0x60) + 0xF91 == hudB) escribirByte(hudB, hudOrigB);
        hudOculto = false;
    }
}

void iniciarSesionFoto(int npc) {
    if (ses.npc >= 0) return;
    ses = SesionFoto{}; ses.npc = npc;
}

void guardarRetrato(int npc, const std::vector<uint32_t>& out) {
    const int S = S_FOTO;
    if (fotos[npc]) fotos[npc]->Release();
    fotos[npc] = crearTextura(S, S, out.data());
    wchar_t ruta[MAX_PATH]; swprintf(ruta, MAX_PATH, L"%ls\\fotos\\npc%02d.bmp", carpetaPartida.c_str(), npc);
    if (FILE* f = carpetaPartida.empty() ? nullptr : _wfopen(ruta, L"wb")) {
        int fila = S * 3;
        BITMAPFILEHEADER fh{0x4D42, (DWORD)(54 + fila * S), 0, 0, 54};
        BITMAPINFOHEADER ih{sizeof ih, S, S, 1, 24, BI_RGB, (DWORD)(fila * S), 2835, 2835, 0, 0};
        fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f);
        for (int y = S - 1; y >= 0; --y)
            for (int x = 0; x < S; ++x) { uint32_t c = out[y * S + x]; uint8_t p[3] = {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c}; fwrite(p, 3, 1, f); }
        fclose(f);
    }
}

// ---------------------------------------------------------------- localizar al NPC en pantalla
struct V3 { float x, y, z; };
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
float punto(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float largo(V3 a) { return sqrtf(punto(a, a)); }

bool posicionChr(uintptr_t chr, V3& p) {
    uintptr_t fis = leerPtr(leerPtr(chr + 0x1F90) + 0x68);   // módulos -> SprjChrPhysicsModule (TGA "X/Z/Y Update")
    if (!fis || !leer(fis + 0x80, p)) return false;
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) && fabsf(p.x) < 1e5f && fabsf(p.y) < 1e5f && fabsf(p.z) < 1e5f;
}

// Personaje más cercano al jugador (sin contar al jugador), recorriendo las listas de WorldChrMan.
bool npcMasCercano(V3 jugador, V3& out, float& dist) {
    uintptr_t wcm = leerPtr(g.sWorldChrMan), yo = leerPtr(wcm + 0x80);
    dist = 1e9f;
    static const uint32_t listas[] = {0x1D0, 0x1E8, 0x1B8, 0x200, 0x218, 0x230, 0x248};
    char resumen[256] = ""; int total = 0;
    for (uint32_t off : listas) {
        uintptr_t lista = leerPtr(wcm + off);
        int n = 0;
        if (!lista || !leer(lista, n) || n <= 0 || n > 2048) continue;
        uintptr_t arr = leerPtr(lista + 8);
        if (!arr) continue;
        int validos = 0;
        for (int i = 0; i <= n; ++i) {
            uintptr_t chr = leerPtr(arr + i * 0x38);
            V3 p;
            if (!chr || chr == yo || !posicionChr(chr, p)) continue;
            ++validos;
            float d = largo(p - jugador);
            if (d > 0.3f && d < dist) { dist = d; out = p; }
        }
        total += validos;
        snprintf(resumen + strlen(resumen), sizeof resumen - strlen(resumen), " 0x%X:%d/%d", off, validos, n);
    }
    logf("Foto: personajes leídos%s; el más cercano a %.1f m", resumen, dist);
    return dist < 8.f;
}

struct Camara { V3 r, u, f, p; float fov; bool ok; };

// Lee una cámara (matriz 4x4 en +0x10, FovY en +0x50). variante: 0 filas, 1 columnas; signo del eje Z.
Camara leerCamara(uintptr_t cam, int variante, float signo) {
    Camara c{}; float m[16]; float fov = 0;
    if (!cam || !leer(cam + 0x10, m) || !leer(cam + 0x50, fov)) return c;
    if (variante == 0) { c.r = {m[0], m[1], m[2]}; c.u = {m[4], m[5], m[6]}; c.f = {m[8], m[9], m[10]}; c.p = {m[12], m[13], m[14]}; }
    else               { c.r = {m[0], m[4], m[8]}; c.u = {m[1], m[5], m[9]}; c.f = {m[2], m[6], m[10]}; c.p = {m[12], m[13], m[14]}; }
    c.f = {c.f.x * signo, c.f.y * signo, c.f.z * signo};
    if (fov > 3.2f) fov *= 3.14159265f / 180.f;            // venía en grados
    c.fov = fov;
    auto unit = [](V3 v) { float l = largo(v); return l > 0.9f && l < 1.1f; };
    c.ok = unit(c.r) && unit(c.u) && unit(c.f) && fov > 0.2f && fov < 2.5f;
    return c;
}

bool proyectar(const Camara& c, V3 P, int W, int H, float& sx, float& sy) {
    V3 v = P - c.p;
    float z = punto(v, c.f);
    if (z < 0.1f) return false;
    float t = tanf(c.fov / 2), asp = W / (float)H;
    float nx = punto(v, c.r) / (z * t * asp), ny = punto(v, c.u) / (z * t);
    sx = (nx + 1) * 0.5f * W; sy = (1 - ny) * 0.5f * H;
    return std::isfinite(sx) && std::isfinite(sy);
}

// Elige la cámara y variante que ponen al jugador cerca del centro de la pantalla.
bool mejorCamara(V3 jugador, int W, int H, Camara& out) {
    uintptr_t fa = leerPtr(g.sFieldArea);
    uintptr_t cams[3] = {leerPtr(leerPtr(leerPtr(fa + 0x18) + 0xE8)), leerPtr(leerPtr(fa + 0x28) + 0x60), leerPtr(leerPtr(fa + 0x28) + 0x68)};
    float mejor = 1e9f; bool hay = false; int elegida = -1, var = -1; float sig = 0;
    for (int k = 0; k < 3; ++k)
        for (int v = 0; v < 2; ++v)
            for (float sg : {1.f, -1.f}) {
                Camara c = leerCamara(cams[k], v, sg);
                if (!c.ok || largo(c.p - jugador) > 15.f) continue;
                float x, y;
                V3 pecho = {jugador.x, jugador.y + 1.0f, jugador.z};
                if (!proyectar(c, pecho, W, H, x, y)) continue;
                float e = fabsf(x / W - 0.5f) + fabsf(y / H - 0.55f);
                if (e < mejor) { mejor = e; out = c; hay = true; elegida = k; var = v; sig = sg; }
            }
    static int ultimaLog = -2;
    if (elegida * 10 + var != ultimaLog) { logf("Cámara: %d variante %d signo %+.0f error %.2f", elegida, var, sig, mejor); ultimaLog = elegida * 10 + var; }
    return hay && mejor < 0.45f;
}

// Guarda una imagen pequeña de depuración con marcas (jugador azul, NPC rojo, encuadre verde).
void guardarDepuracion(const uint8_t* datos, UINT pitch, bool bgra, int W, int H, float jx, float jy, float nx, float ny, int cx0, int cy0, int clado, int npc) {
    const int w = 640, h = 360;
    std::vector<uint8_t> img(w * h * 3);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const uint8_t* p = datos + (y * H / h) * pitch + (x * W / w) * 4;
            uint8_t* q = &img[(y * w + x) * 3];
            q[0] = bgra ? p[0] : p[2]; q[1] = p[1]; q[2] = bgra ? p[2] : p[0];
        }
    auto pix = [&](int x, int y, uint8_t r, uint8_t g_, uint8_t b) { if (x >= 0 && y >= 0 && x < w && y < h) { uint8_t* q = &img[(y * w + x) * 3]; q[0] = b; q[1] = g_; q[2] = r; } };
    auto cruz = [&](float X, float Y, uint8_t r, uint8_t g_, uint8_t b) { int x = (int)(X * w / W), y = (int)(Y * h / H); for (int k = -8; k <= 8; ++k) { pix(x + k, y, r, g_, b); pix(x, y + k, r, g_, b); pix(x + k, y + 1, r, g_, b); pix(x + 1, y + k, r, g_, b); } };
    if (jx >= 0) cruz(jx, jy, 40, 120, 255);
    if (nx >= 0) cruz(nx, ny, 255, 40, 40);
    int x0 = cx0 * w / W, y0 = cy0 * h / H, l = clado * w / W, lh = clado * h / H;
    for (int k = 0; k <= l; ++k) { pix(x0 + k, y0, 40, 220, 40); pix(x0 + k, y0 + lh, 40, 220, 40); }
    for (int k = 0; k <= lh; ++k) { pix(x0, y0 + k, 40, 220, 40); pix(x0 + l, y0 + k, 40, 220, 40); }
    std::wstring dir = std::wstring(carpetaMod.begin(), carpetaMod.end()) + L"\\depuracion";
    CreateDirectoryW(dir.c_str(), nullptr);
    wchar_t ruta[MAX_PATH]; swprintf(ruta, MAX_PATH, L"%ls\\foto_npc%02d.bmp", dir.c_str(), npc);
    if (FILE* f = _wfopen(ruta, L"wb")) {
        int fila = w * 3;
        BITMAPFILEHEADER fh{0x4D42, (DWORD)(54 + fila * h), 0, 0, 54};
        BITMAPINFOHEADER ih{sizeof ih, w, h, 1, 24, BI_RGB, (DWORD)(fila * h), 2835, 2835, 0, 0};
        fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f);
        for (int y = h - 1; y >= 0; --y) fwrite(&img[y * fila], 1, fila, f);
        fclose(f);
    }
}

// Una toma: copia la pantalla, prueba varios encuadres y guarda el mejor hasta ahora.
void evaluarToma(IDXGISwapChain* sc) {
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) return;
    D3D11_TEXTURE2D_DESC d; bb->GetDesc(&d);
    bool bgra = d.Format == DXGI_FORMAT_B8G8R8A8_UNORM || d.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    bool rgba = d.Format == DXGI_FORMAT_R8G8B8A8_UNORM || d.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    if ((!bgra && !rgba) || d.SampleDesc.Count != 1) {
        logf("Foto: formato de pantalla no soportado (%d, msaa %u)", d.Format, d.SampleDesc.Count);
        bb->Release(); ses.tomas = FOTO_TOMAS; return;
    }
    static ID3D11Texture2D* st = nullptr; static UINT stW = 0, stH = 0; static DXGI_FORMAT stF = DXGI_FORMAT_UNKNOWN;
    if (!st || stW != d.Width || stH != d.Height || stF != d.Format) {
        if (st) st->Release();
        D3D11_TEXTURE2D_DESC sd = d;
        sd.Usage = D3D11_USAGE_STAGING; sd.BindFlags = 0; sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ; sd.MiscFlags = 0;
        st = nullptr;
        if (FAILED(dev->CreateTexture2D(&sd, nullptr, &st))) { bb->Release(); return; }
        stW = d.Width; stH = d.Height; stF = d.Format;
    }
    ctx->CopyResource(st, bb);
    bb->Release();
    D3D11_MAPPED_SUBRESOURCE m;
    if (FAILED(ctx->Map(st, 0, D3D11_MAP_READ, 0, &m))) return;
    int W = d.Width, H = d.Height;
    // 1) ¿dónde está el NPC en pantalla? (posición 3D proyectada con la cámara del juego)
    float centroX = 0.5f * W, centroY = 0.36f * H, jx = -1, jy = -1, nx = -1, ny = -1;
    int lado = (int)(H * 0.36f);
    bool proyectado = false;
    V3 yo;
    uintptr_t pl = leerPtr(leerPtr(g.sWorldChrMan) + 0x80);
    Camara cam;
    V3 npc; float dist;
    bool okYo = false, okCam = false, okNpc = false;
    if (!demo && ses.tomas == 0) {
        okYo = pl && posicionChr(pl, yo);
        okCam = okYo && mejorCamara(yo, W, H, cam);
        okNpc = okCam && npcMasCercano(yo, npc, dist);
        if (okYo) logf("Foto: tu posición %.2f %.2f %.2f", yo.x, yo.y, yo.z);
        logf("Foto: posición propia %s, cámara %s, NPC %s", okYo ? "ok" : "FALLÓ", okCam ? "ok" : "FALLÓ", okNpc ? "ok" : "FALLÓ");
    } else if (!demo) {
        okYo = pl && posicionChr(pl, yo); okCam = okYo && mejorCamara(yo, W, H, cam);
        okNpc = okCam && ses.npcOk;            // reutiliza el NPC de la primera toma
        npc = {ses.npcPos[0], ses.npcPos[1], ses.npcPos[2]}; dist = ses.npcDist;
    }
    if (okNpc && ses.tomas == 0) { ses.npcPos[0] = npc.x; ses.npcPos[1] = npc.y; ses.npcPos[2] = npc.z; ses.npcDist = dist; ses.npcOk = true; }
    if (okNpc) {
        float pxs, pys, cxs_, cys_;
        V3 pies = npc, cabeza = {npc.x, npc.y + 1.75f, npc.z};
        proyectar(cam, {yo.x, yo.y + 1.0f, yo.z}, W, H, jx, jy);
        if (proyectar(cam, pies, W, H, pxs, pys) && proyectar(cam, cabeza, W, H, cxs_, cys_)) {
            float altoPant = fabsf(pys - cys_);
            nx = (pxs + cxs_) / 2;
            float deseado = altoPant * 1.15f, maximo = H * 0.78f;
            if (deseado <= maximo) {
                // cabe entero: centro del cuerpo (de pie o sentado, queda dentro)
                ny = (pys + cys_) / 2;
                lado = (int)fmaxf(H * 0.22f, deseado);
            } else {
                // muy cerca: cabeza y torso
                lado = (int)maximo;
                ny = cys_ + lado * 0.42f;
            }
            if (nx > 0 && nx < W && ny > -H * 0.2f && ny < H * 1.2f && altoPant > H * 0.05f) {
                centroX = nx; centroY = ny;
                proyectado = true;
            }
        }
        if (ses.tomas == 0) logf("Foto: NPC a %.1f m, en pantalla (%.0f, %.0f), alto %.0f px, %s", dist, nx, ny, fabsf(pys - cys_), proyectado ? "encuadre por posición" : "posición no válida, encuadre central");
    }
    // 2) probar encuadres alrededor de ese punto y quedarse con el de más detalle
    float mejorLocal = -1; int bx = 0, by = 0;
    const float ox[] = {0.f, -0.12f, 0.12f, -0.25f, 0.25f}, oy[] = {0.f, -0.12f, 0.12f};
    for (float dy : oy)
        for (float dx : ox) {
            int x0 = (int)(centroX + dx * lado) - lado / 2, y0 = (int)(centroY + dy * lado) - lado / 2;
            if (x0 < 0) x0 = 0; if (x0 + lado > W) x0 = W - lado;
            if (y0 < 0) y0 = 0;
            if (y0 + lado > H) y0 = std::max(0, H - lado);
            float sc_ = puntuarEncuadre((const uint8_t*)m.pData, m.RowPitch, bgra, W, H, x0, y0, lado);
            sc_ *= 1.f - (fabsf(dx) + fabsf(dy)) * 1.5f;                     // preferir el punto calculado
            {                                                                 // evitar barras, objetos y almas (por si siguen visibles)
                auto solape = [&](float ax, float ay, float bx_, float by_) {
                    float ix = fmaxf(0.f, fminf(x0 + lado, bx_ * W) - fmaxf((float)x0, ax * W));
                    float iy = fmaxf(0.f, fminf(y0 + lado, by_ * H) - fmaxf((float)y0, ay * H));
                    return ix * iy / (float)(lado * lado);
                };
                float hud = solape(0, 0, 0.45f, 0.13f) + solape(0, 0.55f, 0.20f, 1) + solape(0.80f, 0.86f, 1, 1);
                sc_ *= fmaxf(0.f, 1.f - hud * 2.5f);
            }
            if (sc_ > mejorLocal) { mejorLocal = sc_; bx = x0; by = y0; }
        }
    if (mejorLocal > ses.mejor) {
        ses.mejor = mejorLocal;
        ses.retrato = crearRetrato((const uint8_t*)m.pData, m.RowPitch, bgra, W, H, bx, by, lado);
        if (diagnostico && !demo) guardarDepuracion((const uint8_t*)m.pData, m.RowPitch, bgra, W, H, jx, jy, nx, ny, bx, by, lado, ses.npc);
    }
    ctx->Unmap(st, 0);
}

void avanzarSesionFoto(IDXGISwapChain* sc) {
    if (ses.npc < 0) return;
    ++ses.cuadro;
    if (ses.cuadro == 1) { if (!demo) { ocultarJugador(true); ocultarInterfaz(true); } return; }
    if (ses.cuadro < FOTO_ESPERA) return;
    if ((ses.cuadro - FOTO_ESPERA) % FOTO_CADA == 0 && ses.tomas < FOTO_TOMAS) { evaluarToma(sc); ++ses.tomas; }
    if (ses.tomas >= FOTO_TOMAS) {
        ocultarJugador(false);
        ocultarInterfaz(false);
        if (!ses.retrato.empty()) { guardarRetrato(ses.npc, ses.retrato); logf("Foto de %s guardada (mejor puntuación %.1f)", NPCS[ses.npc].nombre, ses.mejor); }
        ses.npc = -1;
    }
}

// ---------------------------------------------------------------- sonido de página
std::vector<uint8_t> sonApertura, sonPagina[3];
int sonSiguiente = 0;
void prepararSonidos() {
    if (volumen <= 0) return;
    sonApertura = sonido::paginaWav(0, true, volumen);
    for (int i = 0; i < 3; ++i) sonPagina[i] = sonido::paginaWav(i, false, volumen);
}
void sonar(bool apertura) {
    if (volumen <= 0) return;
    const auto& w = apertura ? sonApertura : sonPagina[sonSiguiente++ % 3];
    static bool registrado = false;
    BOOL ok = PlaySoundA((LPCSTR)w.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    if (!registrado || !ok) { logf("Sonido de página: %s", ok ? "ok" : "FALLÓ"); registrado = true; }
}

// ---------------------------------------------------------------- lógica de juego
bool F(uint32_t id) { return g.flag(id); }
bool algunaHoguera(int z) { for (uint32_t h : ZONAS[z].hogueras) { if (!h) break; if (F(h)) return true; } return false; }
bool zonaDescubierta(int z) { return zonasVisitadas.count(z) || algunaHoguera(z); }
bool FB(const FlagBruto& fb) { bool v = false; return fb.bit <= 7 && g.flagBruto(fb.off, fb.bit, v) && v; }
// ---------------------------------------------------------------- misiones con datos del juego
const char* NOMBRE_MAPA[] = {"", u8"el Gran muro de Lothric", u8"el Castillo de Lothric", u8"el Asentamiento de no muertos",
    u8"el Pico del Archidragón", u8"el Camino de los Sacrificios o el Torreón de Farron", u8"el Gran Archivo",
    u8"la Catedral de la Oscuridad", u8"Irithyll o Anor Londo", u8"las Catacumbas de Carthus o el Lago Ardiente",
    u8"la Mazmorra de Irithyll o la Capital Profanada", u8"el Santuario de Enlace del Fuego", u8"el Horno de la Primera Llama",
    u8"el Mundo Pintado de Ariandel", u8"el montón de residuos", u8"la Ciudad Anillada"};
int mapaDeArea(int area) {   // área de dos cifras (p. ej. 40) -> índice de NOMBRE_MAPA
    switch (area) { case 30: return 1; case 31: return 3; case 32: return 4; case 33: return 5; case 34: return 6; case 35: return 7;
                    case 37: return 8; case 38: return 9; case 39: return 10; case 40: return 11; case 41: return 12; case 45: return 13;
                    case 50: return 14; case 51: return 15; default: return 0; }
}
// 93xx = jefe derrotado (extraído de los eventos de muerte de cada jefe)
int jefeDe93(uint32_t f) {
    static const uint32_t t[][2] = {{9300, J_BAILARINA}, {9301, J_VORDT}, {9302, J_OCEIROS}, {9303, J_ARBOL}, {9304, J_REY_SIN_NOMBRE},
        {9305, J_WYVERN}, {9306, J_SABIO}, {9307, J_VIGILANTES}, {9308, J_ARMADURA}, {9309, J_PRINCIPES}, {9311, J_DIACONOS},
        {9313, J_PONTIFICE}, {9314, J_ALDRICH}, {8260, J_ALDRICH}, {9315, J_WOLNIR}, {9317, J_REY_DEMONIO}, {9318, J_YHORM},
        {9319, J_IUDEX}, {9320, J_CAMPEON_GUNDYR}, {9321, J_ALMA_CENIZAS}, {9322, J_FRIEDE}, {9323, J_CUSTODIO},
        {9324, J_PRINCIPE_DEMONIO}, {9325, J_PENUMBRA}, {9326, J_MIDIR}, {9327, J_GAEL}};
    for (auto& x : t) if (x[0] == f) for (int i = 0; i < (int)(sizeof JEFES / sizeof JEFES[0]); ++i) if (JEFES[i].flag == x[1]) return i;
    return -1;
}
int npcDeBloque(uint32_t f) { for (int n = 0; n < N_TOTAL; ++n) { int b = bloqueNpc(n); if (b && f >= (uint32_t)b && f < (uint32_t)b + 20) return n; } return -1; }

int faseActual(int n) {
    int b = bloqueNpc(n);
    if (!b) return 0;
    for (int f = b; f < b + 15; ++f) if (F(f)) return f;
    return 0;
}
const char* lugarDeFase(int fase) { for (auto& x : FASES_LUGAR) if (x.fase == fase) return NOMBRE_MAPA[x.mapa]; return nullptr; }
int profundidad(int fase) { for (auto& x : FASES_PROF) if (x.fase == fase) return x.prof; return -1; }
int pasosTotales(int b) { int m = -1; for (auto& x : FASES_PROF) if (x.fase >= b && x.fase < b + 15 && x.prof > m) m = x.prof; return m + 1; }
bool tieneSalida(int fase) { for (auto& t : TRANSICIONES) if (t.desde == fase) return true; return false; }
bool conocido(int n);

// Traduce las condiciones de la siguiente transición a una pista ("" si ninguna es legible).
std::string paraAvanzar(int n, int fase) {
    for (auto& t : TRANSICIONES) {
        if (t.desde != fase) continue;
        std::string r;
        for (int k = 0; k < 4 && t.cond[k]; ++k) {
            uint32_t f = t.cond[k]; bool on = t.estado[k];
            std::string parte;
            if (int j = jefeDe93(f); j >= 0) {
                bool visto = zonaDescubierta(JEFES[j].zona) || F(JEFES[j].flag);
                std::string nom = visto ? JEFES[j].nombre : u8"cierto jefe";
                parte = on ? u8"derrotar a " + nom : u8"antes de derrotar a " + nom;
            } else if (f >= 70000000 && f < 80000000 && on) {
                int mapa = mapaDeArea((f / 100000) % 100);
                parte = std::string(u8"hablar con ") + (n >= 0 ? u8"esta persona" : "") + (mapa ? std::string(u8" en ") + NOMBRE_MAPA[mapa] : "");
            } else if (int o = npcDeBloque(f); o >= 0 && o != n && (f - bloqueNpc(o)) == 18) {
                if (conocido(o)) parte = on ? std::string(u8"que ") + NPCS[o].nombre + u8" haya muerto" : std::string(u8"que ") + NPCS[o].nombre + u8" siga con vida";
                else parte = u8"algo que depende de alguien que aún no conoces";
            }
            if (parte.empty() || r.find(parte) != std::string::npos) continue;   // sin repetir
            if (!r.empty()) r += u8" y ";
            r += parte;
        }
        if (!r.empty()) return r;
    }
    return "";
}

bool muertoNpc(int n) { int b = bloqueNpc(n); return FB(NPCS[n].muerto) || (b && F(b + 18)); }
bool conocido(int n) { return npcsConocidos.count(n) || FB(NPCS[n].hablado) || FB(NPCS[n].enSantuario); }

struct Objetivo { const char* zona; const char* texto; };

Objetivo objetivoPrincipal(std::vector<std::string>& extra) {
    auto opc = [&](bool cond, const char* t) { if (cond && extra.size() < 3) extra.push_back(t); };
    bool asent = algunaHoguera(Z_ASENTAMIENTO), cata = algunaHoguera(Z_CATACUMBAS);
    opc(asent && !F(J_ARBOL), u8"Opcional: Gran Árbol corrompido, en el Asentamiento de no muertos.");
    opc(F(J_WOLNIR) && !F(J_REY_DEMONIO) && algunaHoguera(Z_LAGO), u8"Opcional: Viejo rey demonio, en el Lago Ardiente.");
    opc(F(J_BAILARINA) && !F(J_OCEIROS), u8"Opcional: Oceiros, en el Jardín del Rey Consumido.");
    opc(F(J_OCEIROS) && !F(J_CAMPEON_GUNDYR), u8"Opcional: algo aguarda tras un muro ilusorio cerca de Oceiros.");
    opc(F(J_OCEIROS) && !F(J_REY_SIN_NOMBRE), u8"Opcional: el gesto «Camino del dragón» abre el Pico del Archidragón.");

    if (!F(J_IUDEX))   return {u8"Cementerio de Ceniza", u8"Sube por el cementerio y derrota a Iudex Gundyr."};
    if (!F(14000000))  return {u8"Santuario de Enlace del Fuego", u8"Entra en el Santuario y clava la Espada en espiral en su hoguera."};
    if (!F(J_VORDT))   return {u8"Gran muro de Lothric", u8"Viaja al Gran muro desde la hoguera del Santuario y abre camino hasta Vordt del Valle Boreal."};
    if (!asent)        return {u8"Asentamiento de no muertos", u8"Habla con Emma en la capilla del Gran muro y alza el Estandarte peq. de Lothric en la puerta principal del castillo."};
    if (!algunaHoguera(Z_CAMINO)) return {u8"Camino de los Sacrificios", u8"Cruza el Asentamiento de no muertos hasta llegar al Camino de los Sacrificios."};
    if (!F(J_SABIO))   return {u8"Camino de los Sacrificios", u8"Atraviesa los Bosques de crucifixión y derrota al Sabio de cristal."};
    if (!F(J_VIGILANTES)) {
        opc(!F(J_DIACONOS), u8"También: en la Catedral de la Oscuridad, los Diáconos guardan el Muñeco pequeño, necesario para entrar en Irithyll.");
        return {u8"Torreón de Farron", u8"Apaga las tres llamas del torreón y derrota a los Vigilantes del Abismo."};
    }
    if (!F(J_DIACONOS)) return {u8"Catedral de la Oscuridad", u8"Llega al fondo de la Catedral y derrota a los Diáconos: te darán el Muñeco pequeño."};
    if (!algunaHoguera(Z_IRITHYLL)) {
        opc(cata && !F(J_WOLNIR), u8"En lo profundo de las Catacumbas espera el Gran Señor Wolnir.");
        return {u8"Irithyll del Valle Boreal", u8"Tras los Vigilantes, cruza las Catacumbas de Carthus. Al otro lado, el Muñeco pequeño te dejará pasar a Irithyll."};
    }
    if (!F(J_PONTIFICE)) return {u8"Irithyll del Valle Boreal", u8"Atraviesa Irithyll hasta su iglesia y derrota al Pontífice Sulyvahn."};
    if (!F(J_ALDRICH) || !F(J_YHORM)) {
        if (!F(J_ALDRICH) && !F(J_YHORM))
            opc(true, u8"También: baja a la Mazmorra de Irithyll y sigue hasta la Capital Profanada (Yhorm, el Gigante).");
        if (!F(J_ALDRICH)) return {u8"Anor Londo", u8"Tras el Pontífice, sube a Anor Londo y derrota a Aldrich, el Devoradioses."};
        return {u8"Capital Profanada", u8"Atraviesa la Mazmorra de Irithyll hasta la Capital Profanada y derrota a Yhorm, el Gigante."};
    }
    if (!F(J_BAILARINA)) return {u8"Gran muro de Lothric", u8"Vuelve a la capilla de Emma en el Gran muro: allí obtendrás la Jofaina de votos. Úsala en la estatua del caballero."};
    if (!F(J_ARMADURA)) return {u8"Castillo de Lothric", u8"Sube la escalera tras la Bailarina, cruza el Castillo de Lothric y derrota a la Armadura del Asesino de dragones."};
    if (!F(J_PRINCIPES)) return {u8"Gran Archivo", u8"Abre el Gran Archivo con su llave y llega hasta los Príncipes Gemelos."};
    if (!F(J_ALMA_CENIZAS)) return {u8"Horno de la Primera Llama", u8"Lleva las Cenizas de un Señor a los tronos del Santuario y habla con la Guardiana del Fuego."};
    return {u8"Historia completada", u8"Has terminado la historia principal. Quedan las zonas opcionales que aún no hayas visto."};
}

// Hitos de la historia principal, en orden (para la hoja de misiones).
struct Hito { const char* texto; bool hecho; };
std::vector<Hito> hitosPrincipales() {
    return {
        {u8"Derrotar a Iudex Gundyr", F(J_IUDEX)},
        {u8"Encender la hoguera del Santuario", F(14000000)},
        {u8"Derrotar a Vordt del Valle Boreal", F(J_VORDT)},
        {u8"Llegar al Asentamiento de no muertos", algunaHoguera(Z_ASENTAMIENTO)},
        {u8"Llegar al Camino de los Sacrificios", algunaHoguera(Z_CAMINO)},
        {u8"Derrotar al Sabio de cristal", F(J_SABIO)},
        {u8"Derrotar a los Vigilantes del Abismo", F(J_VIGILANTES)},
        {u8"Conseguir el Muñeco pequeño (Diáconos)", F(J_DIACONOS)},
        {u8"Llegar a Irithyll del Valle Boreal", algunaHoguera(Z_IRITHYLL)},
        {u8"Derrotar al Pontífice Sulyvahn", F(J_PONTIFICE)},
        {u8"Derrotar a Aldrich, el Devoradioses", F(J_ALDRICH)},
        {u8"Derrotar a Yhorm, el Gigante", F(J_YHORM)},
        {u8"Derrotar a la Bailarina del Valle Boreal", F(J_BAILARINA)},
        {u8"Derrotar a la Armadura del Asesino de dragones", F(J_ARMADURA)},
        {u8"Derrotar a los Príncipes Gemelos", F(J_PRINCIPES)},
        {u8"Avivar la Primera Llama: Alma de Cenizas", F(J_ALMA_CENIZAS)},
    };
}

// Paso actual de un NPC: el último paso desbloqueado por el progreso.
int pasoActual(const Npc& n) {
    int act = 0;
    for (int i = 0; i < 8 && n.pasos[i].texto; ++i)
        if (!n.pasos[i].requiere || F(n.pasos[i].requiere)) act = i;
    return act;
}

// ---------------------------------------------------------------- entrada
bool teclaPulsada(int vk) {
    static bool prev[256] = {};
    bool ahora = (GetAsyncKeyState(vk) & 0x8000) != 0 && GetForegroundWindow() == hwnd;
    bool r = ahora && !prev[vk & 255];
    prev[vk & 255] = ahora;
    return r;
}
using XInputGetState_t = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
XInputGetState_t pXInput = nullptr;          // lectura real del mando (sin filtrar)
volatile bool ocultarSticks = false;         // con el libro abierto, el juego no ve L3/R3

DWORD WINAPI hkXInputGetState(DWORD i, XINPUT_STATE* s) {
    DWORD r = pXInput(i, s);
    if (r == ERROR_SUCCESS && s && ocultarSticks)
        s->Gamepad.wButtons &= ~(mandoAbrir | mandoSiguiente | mandoAnterior);
    return r;
}

void prepararMando() {
    static bool hecho = false;
    if (hecho) return;
    hecho = true;
    for (const wchar_t* dll : {L"xinput1_3", L"xinput1_4", L"xinput9_1_0"}) {
        if (!GetModuleHandleW((std::wstring(dll) + L".dll").c_str()) && wcscmp(dll, L"xinput1_3")) continue;
        LoadLibraryW((std::wstring(dll) + L".dll").c_str());
        void* orig = nullptr;
        if (MH_CreateHookApi(dll, "XInputGetState", (void*)hkXInputGetState, &orig) == MH_OK) {
            void* destino = (void*)GetProcAddress(GetModuleHandleW((std::wstring(dll) + L".dll").c_str()), "XInputGetState");
            if (MH_EnableHook(destino) == MH_OK && !pXInput) pXInput = (XInputGetState_t)orig;
            logf("Mando: gancho en %ls %s", dll, pXInput ? "ok" : "falló");
        }
    }
    if (!pXInput) {   // sin gancho: al menos leer el mando (el juego seguirá viendo L3/R3)
        if (HMODULE m = LoadLibraryA("xinput1_3.dll")) pXInput = (XInputGetState_t)GetProcAddress(m, "XInputGetState");
        logf("Mando: sin gancho, solo lectura (%s)", pXInput ? "ok" : "no disponible");
    }
}

// ---------------------------------------------------------------- diálogo (gancho a la función de textos)
using ObtenerMensaje_t = const wchar_t*(__fastcall*)(void*, int, int, int);
ObtenerMensaje_t oObtenerMensaje = nullptr;
volatile int ultimoDialogo = -1;
volatile ULONGLONG ultimoDialogoTick = 0;
const wchar_t* __fastcall hkObtenerMensaje(void* repo, int v, int cat, int id) {
    if ((cat == 1 || cat == 230 || cat == 270) && id >= 1000000) { ultimoDialogo = id; ultimoDialogoTick = GetTickCount64(); }
    return oObtenerMensaje(repo, v, cat, id);
}
void engancharDialogos() {
    if (!g.fObtenerMensaje) return;
    bool ok = MH_CreateHook((void*)g.fObtenerMensaje, (void*)hkObtenerMensaje, (void**)&oObtenerMensaje) == MH_OK &&
              MH_EnableHook((void*)g.fObtenerMensaje) == MH_OK;
    logf("Detección de diálogos: %s", ok ? "ok" : "FALLÓ");
}
// Diálogo "activo": última línea pedida hace menos de 4 s
int dialogoReciente() {
    if (ultimoDialogo < 0 || GetTickCount64() - ultimoDialogoTick > 4000) return -1;
    return ultimoDialogo;
}

// Último dispositivo usado (para mostrar solo sus controles): true = mando, false = teclado/ratón.
bool usandoMando = false;
void detectarDispositivo() {
    if (pXInput) {
        static DWORD paquete[4] = {};
        for (DWORD i = 0; i < 4; ++i) {
            XINPUT_STATE st{};
            if (pXInput(i, &st) != ERROR_SUCCESS) continue;
            const XINPUT_GAMEPAD& g = st.Gamepad;
            bool activo = g.wButtons || g.bLeftTrigger > 40 || g.bRightTrigger > 40 ||
                          abs(g.sThumbLX) > 9000 || abs(g.sThumbLY) > 9000 || abs(g.sThumbRX) > 9000 || abs(g.sThumbRY) > 9000;
            if (activo && st.dwPacketNumber != paquete[i]) usandoMando = true;
            paquete[i] = st.dwPacketNumber;
        }
    }
    if (GetForegroundWindow() != hwnd) return;
    for (int vk = 0x08; vk <= 0xFE; ++vk)
        if (GetAsyncKeyState(vk) & 0x8000) { usandoMando = false; return; }
    static POINT ultimo{};
    POINT p; GetCursorPos(&p);
    if (abs(p.x - ultimo.x) + abs(p.y - ultimo.y) > 12) { if (ultimo.x || ultimo.y) usandoMando = false; ultimo = p; }
}

// Devuelve 0 nada, 1 abrir/cerrar, 2 siguiente, 3 anterior. Decide al soltar los botones,
// para que L3+R3 no cuente como L3 o R3 sueltos si uno llega antes que el otro.
int accionMando() {
    static WORD acumulado = 0;
    if (!pXInput) return 0;
    WORD todos = mandoAbrir | mandoSiguiente | mandoAnterior, ahora = 0;
    for (DWORD i = 0; i < 4; ++i) {
        XINPUT_STATE st{};
        if (pXInput(i, &st) == ERROR_SUCCESS) ahora |= st.Gamepad.wButtons & todos;
    }
    if (ahora) { acumulado |= ahora; return 0; }
    if (!acumulado) return 0;
    WORD a = acumulado; acumulado = 0;
    if ((a & mandoAbrir) == mandoAbrir) return 1;
    if (a == mandoSiguiente) return 2;
    if (a == mandoAnterior) return 3;
    return 0;
}

// ---------------------------------------------------------------- dibujo
const ImU32 TINTA = IM_COL32(58, 38, 22, 255), TINTA_SUAVE = IM_COL32(96, 70, 45, 255), ROJO = IM_COL32(128, 30, 22, 255);

void fondoPapel(ImDrawList* dl, ImVec2 a, ImVec2 b, float alfa) {
    dl->AddRectFilled(ImVec2(a.x + 5, a.y + 6), ImVec2(b.x + 5, b.y + 6), IM_COL32(0, 0, 0, (int)(90 * alfa)), 6);
    dl->AddImageRounded((ImTextureID)texPapel, a, b, ImVec2(0, 0), ImVec2(1, 1),
                        IM_COL32(255, 255, 255, (int)(255 * alfa)), 6);
    dl->AddRect(a, b, IM_COL32(90, 60, 30, (int)(150 * alfa)), 6, 0, 2);
    dl->AddRect(ImVec2(a.x + 6, a.y + 6), ImVec2(b.x - 6, b.y - 6), IM_COL32(120, 85, 50, (int)(70 * alfa)), 4, 0, 1);
}

void icono(ImDrawList* dl, ImVec2 p, float s, bool aviso) {
    const float t = 46 * s;   // pergamino pixelado (recursos/icono_pergamino.png)
    if (texIcono) dl->AddImage((ImTextureID)texIcono, ImVec2(p.x - 8 * s, p.y), ImVec2(p.x - 8 * s + t, p.y + t));
    if (aviso) dl->AddCircleFilled(ImVec2(p.x + 30 * s, p.y + 6 * s), 5 * s, ROJO);
}

// ---------------------------------------------------------------- libro: texto sobre draw lists
float E = 1.f;   // escala de la interfaz (alto de pantalla / 1080 * escala)

inline ImU32 conAlfa(ImU32 c, float a) {
    int al = (int)(((c >> IM_COL32_A_SHIFT) & 0xFF) * fminf(1.f, fmaxf(0.f, a)));
    return (c & ~IM_COL32_A_MASK) | ((ImU32)al << IM_COL32_A_SHIFT);
}

// Escribe texto con ajuste de línea en un rectángulo, sobre cualquier ImDrawList.
struct Lienzo {
    ImDrawList* dl; float x0, x, y, w, alfa; ImVec4 clip;
    void empezar() { dl->PushClipRect(ImVec2(clip.x, clip.y), ImVec2(clip.z, clip.w), true); }
    void terminar() { dl->PopClipRect(); }
    void texto(ImFont* f, ImU32 c, const char* s, float tras = 0) {
        float ancho = w - (x - x0);
        ImVec2 sz = f->CalcTextSizeA(f->FontSize, FLT_MAX, ancho, s);
        dl->AddText(f, f->FontSize, ImVec2(x, y), conAlfa(c, alfa), s, nullptr, ancho, &clip);
        y += sz.y + tras;
    }
    void centrado(ImFont* f, ImU32 c, const char* s) {
        float tw = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0, s).x;
        dl->AddText(f, f->FontSize, ImVec2(x0 + (w - tw) / 2, y), conAlfa(c, alfa), s, nullptr, 0, &clip);
        y += f->FontSize * 1.2f;
    }
    void espacio(float h) { y += h * E; }
    void separador() {
        dl->AddLine(ImVec2(x0 + w * 0.15f, y + 9 * E), ImVec2(x0 + w * 0.85f, y + 9 * E), conAlfa(IM_COL32(110, 80, 50, 150), alfa), 1.5f * E);
        dl->AddCircleFilled(ImVec2(x0 + w * 0.5f, y + 9 * E), 3 * E, conAlfa(IM_COL32(110, 80, 50, 170), alfa));
        y += 20 * E;
    }
};

void marca(ImDrawList* dl, ImVec2 p, float s, bool hecho, float alfa) {
    if (hecho) {
        dl->AddLine(ImVec2(p.x, p.y + 6 * s), ImVec2(p.x + 5 * s, p.y + 11 * s), conAlfa(TINTA, alfa), 2.4f * s);
        dl->AddLine(ImVec2(p.x + 5 * s, p.y + 11 * s), ImVec2(p.x + 13 * s, p.y), conAlfa(TINTA, alfa), 2.4f * s);
    } else dl->AddCircle(ImVec2(p.x + 6 * s, p.y + 6 * s), 4.5f * s, conAlfa(TINTA_SUAVE, alfa), 12, 1.4f * s);
}

std::vector<int> npcsVisibles() {
    std::vector<int> v;
    for (int i = 0; i < N_TOTAL; ++i) if (conocido(i)) v.push_back(i);
    return v;
}
constexpr int NPCS_POR_HOJA = 3;

void hojaZonasIzq(Lienzo& L, int zonaAct) {
    L.centrado(fCursiva, TINTA_SUAVE, u8"~ Cuaderno del Ceniciento ~");
    L.espacio(14);
    L.texto(fPeque, TINTA_SUAVE, u8"Estás en:", 2 * E);
    L.texto(fTitulo, TINTA, zonaAct >= 0 && zonaAct < Z_TOTAL ? ZONAS[zonaAct].nombre : (zonaAct == Z_ARENA ? u8"Arena" : u8"Lugar desconocido"));
    L.separador();
    std::vector<std::string> extra;
    Objetivo o = objetivoPrincipal(extra);
    L.texto(fPeque, TINTA_SUAVE, u8"Debes seguir hacia:", 2 * E);
    L.texto(fTitulo, TINTA, o.zona, 6 * E);
    L.texto(fCursiva, TINTA_SUAVE, o.texto, 10 * E);
    for (auto& e : extra) L.texto(fCursiva, TINTA_SUAVE, e.c_str(), 8 * E);
    L.separador();

    // lista con marcas: dos columnas si hay muchas filas
    auto lista = [&](const std::vector<std::pair<const char*, bool>>& filas) {
        float filaH = fPeque->FontSize * 1.28f, colW = L.w / 2;
        int nf = filas.size() > 5 ? ((int)filas.size() + 1) / 2 : (int)filas.size();
        for (int k = 0; k < (int)filas.size(); ++k) {
            float x = L.x0 + (nf ? k / nf : 0) * colW, y = L.y + (nf ? k % nf : 0) * filaH;
            marca(L.dl, ImVec2(x + 4 * E, y + 7 * E), E, filas[k].second, L.alfa);
            ImVec4 c(x, y, x + (filas.size() > 5 ? colW - 6 * E : L.w), y + filaH);
            L.dl->AddText(fPeque, fPeque->FontSize, ImVec2(x + 28 * E, y), conAlfa(filas[k].second ? TINTA : TINTA_SUAVE, L.alfa),
                          filas[k].second ? filas[k].first : "???", nullptr, 0, &c);
        }
        L.y += nf * filaH;
    };

    // Señores de la Ceniza
    std::vector<std::pair<const char*, bool>> senores = {
        {u8"Vigilantes del Abismo", F(J_VIGILANTES)}, {u8"Aldrich, el Devoradioses", F(J_ALDRICH)},
        {u8"Yhorm, el Gigante", F(J_YHORM)}, {u8"Lothric, joven príncipe", F(J_PRINCIPES)}};
    int ns = 0; for (auto& x : senores) ns += x.second;
    char t[64]; snprintf(t, sizeof t, u8"Señores de la Ceniza (%d/4)", ns);
    L.texto(fTexto, TINTA, t, 4 * E);
    lista(senores);
    L.espacio(10);

    // hogueras de la zona actual
    if (zonaAct >= 0 && zonaAct < Z_TOTAL) {
        std::vector<std::pair<const char*, bool>> hs;
        for (uint32_t h : ZONAS[zonaAct].hogueras) {
            if (!h) break;
            for (auto& n : HOGUERAS) if (n.flag == h) hs.push_back({n.nombre, F(h)});
        }
        int enc = 0; for (auto& x : hs) enc += x.second;
        snprintf(t, sizeof t, u8"Hogueras de esta zona (%d/%d)", enc, (int)hs.size());
        L.texto(fTexto, TINTA, t, 4 * E);
        lista(hs);
    }
}

void hojaZonasDer(Lienzo& L, int zonaAct) {
    int base = 0, total = 0;
    for (int z = 0; z < Z_TOTAL; ++z) { if (!ZONAS[z].dlc) ++total; if (zonaDescubierta(z) && !ZONAS[z].dlc) ++base; }
    char t[96]; snprintf(t, sizeof t, u8"Zonas descubiertas (%d/%d)", base, total);
    L.centrado(fTexto, TINTA, t);
    L.separador();
    float filaH = fPeque->FontSize * 1.28f;
    for (int z = 0; z < Z_TOTAL; ++z) {
        bool d = zonaDescubierta(z);
        if (ZONAS[z].dlc && !d) continue;
        marca(L.dl, ImVec2(L.x + 4 * E, L.y + 7 * E), E, d, L.alfa);
        L.dl->AddText(fPeque, fPeque->FontSize, ImVec2(L.x + 28 * E, L.y), conAlfa(z == zonaAct ? ROJO : (d ? TINTA : TINTA_SUAVE), L.alfa),
                      d ? ZONAS[z].nombre : "???", nullptr, 0, &L.clip);
        L.y += filaH;
    }
}


// ---------------------------------------------------------------- apartados y páginas
// El libro es una lista de páginas; cada apartado ocupa las que necesite y empieza en página izquierda.
enum TipoPagina { P_ZONAS_IZQ, P_ZONAS_DER, P_PRINCIPALES, P_SECUNDARIAS, P_OBJETOS, P_COLECCIONABLES, P_JEFES, P_PERSONAS, P_VACIA };
struct Pagina { int tipo; int desde, hasta; bool primera; };
constexpr int JEFES_POR_PAGINA = 10, SECUNDARIAS_POR_PAGINA = 16, OBJETOS_POR_PAGINA = 8, FILAS_COLECCIONABLES = 17;

std::vector<int> jefesVisibles() {
    bool hayDlc = false;
    for (auto& j : JEFES) if (j.dlc && F(j.flag)) hayDlc = true;
    for (int z = Z_ARIANDEL; z <= Z_ANILLADA; ++z) if (zonaDescubierta(z)) hayDlc = true;
    std::vector<int> v;
    for (int i = 0; i < (int)(sizeof JEFES / sizeof JEFES[0]); ++i) if (!JEFES[i].dlc || hayDlc) v.push_back(i);
    return v;
}

// ---- objetos clave
constexpr int N_OBJETOS = (int)(sizeof OBJETOS / sizeof OBJETOS[0]);
uint32_t idCompleto(const Objeto& o) { return o.tipo == 4 ? 0x40000000u | o.id : o.tipo == 1 ? 0x10000000u | o.id : o.tipo == 2 ? 0x20000000u | o.id : o.id; }
bool coincide(const Objeto& o, uint32_t inv) {
    if (o.tipo == 0) return (inv >> 28) == 0 && inv / 10000 * 10000 == o.id;   // armas: ignora mejora e infusión
    return inv == idCompleto(o);
}
bool loTiene(const Objeto& o) { for (uint32_t i : objetosAhora) if (coincide(o, i)) return true; return false; }
bool loTuvo(const Objeto& o) { return objetosVistos.count(idCompleto(o)) > 0; }
// Objetos de la historia que se consumen: la partida dice si ya se usaron aunque el mod no los viera.
// (Cenizas colocadas en el trono: flags 9211-9214, de la tabla TGA "Activate Lord of Cinder".)
bool usadoSegunPartida(const Objeto& o) {
    switch (o.id) {
    case 2137: return F(14000000);          // Espada en espiral: hoguera del Santuario encendida
    case 2117: return F(J_BAILARINA);       // Jofaina de votos: Bailarina derrotada
    case 2123: return F(9213);              // Cenizas de los Vigilantes en su trono
    case 2124: return F(9212);              // Cenizas de Aldrich
    case 2125: return F(9214);              // Cenizas de Yhorm
    case 2126: return F(9211);              // Cenizas de Lothric
    default: return false;
    }
}
std::vector<int> objetosVisibles() {
    std::vector<int> v;
    for (int i = 0; i < N_OBJETOS; ++i) if (loTiene(OBJETOS[i]) || loTuvo(OBJETOS[i]) || usadoSegunPartida(OBJETOS[i])) v.push_back(i);
    return v;
}
void actualizarInventario() {
    static ULONGLONG ultimo = 0;
    if (GetTickCount64() - ultimo < 2000) return;
    ultimo = GetTickCount64();
    std::set<uint32_t> inv;
    if (!g.inventario(inv)) return;
    objetosAhora = inv;
    for (const Objeto& o : OBJETOS)
        if (loTiene(o) && objetosVistos.insert(idCompleto(o)).second) { estadoSucio = true; logf("Objeto clave: %s", o.nombre); }
    for (const Coleccionable& c : COLECCIONABLES)
        if (objetosAhora.count(0x40000000u | c.id) && objetosVistos.insert(0x40000000u | c.id).second) { estadoSucio = true; logf("Coleccionable: %s", c.nombre); }
}

// ---- coleccionables: filas (encabezado por destinatario o un objeto)
struct FilaCol { int destino; int item; };   // item -1 = encabezado
std::vector<FilaCol> filasColeccionables() {
    std::vector<FilaCol> f;
    for (int d = 0; d < (int)(sizeof DESTINOS / sizeof DESTINOS[0]); ++d) {
        f.push_back({d, -1});
        for (int i = 0; i < (int)(sizeof COLECCIONABLES / sizeof COLECCIONABLES[0]); ++i)
            if (COLECCIONABLES[i].destino == d) f.push_back({d, i});
    }
    return f;
}

std::vector<Pagina> paginasLibro() {
    std::vector<Pagina> p;
    auto apartado = [&](int tipo, int n, int porPagina) {
        if (p.size() % 2) p.push_back({P_VACIA, 0, 0, false});   // empezar en página izquierda
        if (n == 0) { p.push_back({tipo, 0, 0, true}); return; }
        for (int d = 0; d < n; d += porPagina) p.push_back({tipo, d, std::min(n, d + porPagina), d == 0});
    };
    p.push_back({P_ZONAS_IZQ, 0, 0, true});
    p.push_back({P_ZONAS_DER, 0, 0, true});
    p.push_back({P_PRINCIPALES, 0, 0, true});
    int nn = (int)npcsVisibles().size();
    for (int d = 0; d < std::max(nn, 1); d += SECUNDARIAS_POR_PAGINA)
        p.push_back({P_SECUNDARIAS, d, std::min(nn, d + SECUNDARIAS_POR_PAGINA), d == 0});
    apartado(P_OBJETOS, (int)objetosVisibles().size(), OBJETOS_POR_PAGINA);
    apartado(P_COLECCIONABLES, (int)filasColeccionables().size(), FILAS_COLECCIONABLES);
    apartado(P_JEFES, (int)jefesVisibles().size(), JEFES_POR_PAGINA);
    apartado(P_PERSONAS, nn, NPCS_POR_HOJA);
    if (p.size() % 2) p.push_back({P_VACIA, 0, 0, false});
    return p;
}
int totalPliegos() { return (int)paginasLibro().size() / 2; }

void titulo(Lienzo& L, const char* t, bool) {
    L.centrado(fTexto, TINTA, t);
    L.separador();
}

// Fila con marca; si "tachado", una raya cruza el texto.
void filaMarcada(Lienzo& L, ImFont* f, const char* txt, bool marcado, bool tachado, ImU32 col, float alto) {
    marca(L.dl, ImVec2(L.x + 4 * E, L.y + (alto - 12 * E) / 2), E, marcado, L.alfa);
    ImVec2 pos(L.x + 28 * E, L.y + (alto - f->FontSize) / 2);
    L.dl->AddText(f, f->FontSize, pos, conAlfa(col, L.alfa), txt, nullptr, 0, &L.clip);
    if (tachado) {
        float tw = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0, txt).x;
        float y = pos.y + f->FontSize * 0.55f;
        L.dl->AddLine(ImVec2(pos.x - 2 * E, y), ImVec2(pos.x + tw + 2 * E, y + 1 * E), conAlfa(IM_COL32(70, 45, 25, 210), L.alfa), 1.8f * E);
    }
    L.y += alto;
}

void hojaPrincipales(Lienzo& L) {
    auto hitos = hitosPrincipales();
    int hechos = 0; for (auto& h : hitos) hechos += h.hecho;
    char t[64]; snprintf(t, sizeof t, u8"Misiones principales (%d/%d)", hechos, (int)hitos.size());
    titulo(L, t, true);
    float alto = fPeque->FontSize * 1.3f;
    bool actualMostrada = false; int ocultas = 0;
    for (auto& h : hitos) {
        if (h.hecho) filaMarcada(L, fPeque, h.texto, true, true, TINTA_SUAVE, alto);
        else if (!actualMostrada) { filaMarcada(L, fPeque, h.texto, false, false, ROJO, alto); actualMostrada = true; }
        else ++ocultas;
    }
    if (ocultas) { snprintf(t, sizeof t, u8"… y %d más por descubrir", ocultas); L.x += 28 * E; L.texto(fCursiva, TINTA_SUAVE, t); L.x = L.x0; }
}

void hojaSecundarias(Lienzo& L, const Pagina& pg) {
    titulo(L, u8"Misiones secundarias", pg.primera);
    auto v = npcsVisibles();
    if (v.empty()) { L.texto(fCursiva, TINTA_SUAVE, u8"Aún no conoces a nadie. Las misiones de las personas que conozcas aparecerán aquí."); return; }
    float alto = fPeque->FontSize * 1.3f;
    for (int k = pg.desde; k < pg.hasta; ++k) {
        const Npc& n = NPCS[v[k]];
        int total = 0; while (total < 8 && n.pasos[total].texto) ++total;
        int paso = pasoActual(n);
        int idx = (int)(&n - NPCS), fase = faseActual(idx), prof = fase ? profundidad(fase) : -1;
        if (prof >= 0) { paso = prof; total = std::max(pasosTotales(bloqueNpc(idx)), prof + 1); }
        bool final_ = fase && prof > 0 && !tieneSalida(fase);
        bool muerto = muertoNpc(idx) || final_;   // lo que el juego confirma: muerte o fase final
        char linea[160];
        if (muerto) snprintf(linea, sizeof linea, u8"%s (terminó)", n.nombre);
        else if (total <= 1) snprintf(linea, sizeof linea, u8"%s", n.nombre);
        else if (paso == total - 1) snprintf(linea, sizeof linea, u8"%s — último paso", n.nombre);
        else snprintf(linea, sizeof linea, u8"%s — paso %d de %d", n.nombre, paso + 1, total);
        filaMarcada(L, fPeque, linea, muerto, muerto, muerto ? TINTA_SUAVE : TINTA, alto);
    }
}

void hojaObjetos(Lienzo& L, const Pagina& pg) {
    auto v = objetosVisibles();
    char t[64]; snprintf(t, sizeof t, u8"Objetos clave (%d/%d)", (int)v.size(), N_OBJETOS);
    titulo(L, t, pg.primera);
    if (v.empty()) { L.texto(fCursiva, TINTA_SUAVE, u8"Aún no has encontrado objetos clave. Los que consigas aparecerán aquí con su uso."); return; }
    for (int k = pg.desde; k < pg.hasta; ++k) {
        const Objeto& o = OBJETOS[v[k]];
        bool tiene = loTiene(o);
        marca(L.dl, ImVec2(L.x + 4 * E, L.y + 9 * E), E, tiene, L.alfa);
        L.x = L.x0 + 28 * E;
        L.texto(fTexto, tiene ? TINTA : TINTA_SUAVE, o.nombre, 0);
        char sub[128]; snprintf(sub, sizeof sub, u8"%s · %s", o.para, tiene ? u8"lo tienes" : u8"ya lo usaste o entregaste");
        L.texto(fCursiva, tiene ? ROJO : TINTA_SUAVE, sub, 0);
        L.texto(fPeque, TINTA_SUAVE, o.uso, 10 * E);
        L.x = L.x0;
    }
}

void hojaColeccionables(Lienzo& L, const Pagina& pg) {
    auto filas = filasColeccionables();
    int total = 0, hechos = 0, pendientes = 0;
    for (auto& c : COLECCIONABLES) {
        ++total; uint32_t id = 0x40000000u | c.id;
        if (objetosAhora.count(id)) ++pendientes; else if (objetosVistos.count(id)) ++hechos;
    }
    char t[96]; snprintf(t, sizeof t, u8"Coleccionables (%d entregados, %d por entregar)", hechos, pendientes);
    titulo(L, t, pg.primera);
    float alto = fPeque->FontSize * 1.3f;
    for (int k = pg.desde; k < pg.hasta; ++k) {
        const FilaCol& f = filas[k];
        if (k == pg.desde && f.item >= 0) {   // el grupo viene de la página anterior: repetir su encabezado
            char h[128]; snprintf(h, sizeof h, u8"Para %s (%s)", DESTINOS[f.destino], DESBLOQUEA[f.destino]);
            L.texto(fCursiva, TINTA_SUAVE, h, 2 * E);
        }
        if (f.item < 0) {
            char h[128]; snprintf(h, sizeof h, u8"Para %s (%s)", DESTINOS[f.destino], DESBLOQUEA[f.destino]);
            if (k != pg.desde) L.espacio(6);
            L.texto(fCursiva, TINTA_SUAVE, h, 2 * E);
            continue;
        }
        const Coleccionable& c = COLECCIONABLES[f.item];
        uint32_t id = 0x40000000u | c.id;
        bool tiene = objetosAhora.count(id) > 0, entregado = !tiene && objetosVistos.count(id) > 0;
        char linea[160];
        if (tiene) snprintf(linea, sizeof linea, u8"%s — lo tienes: llévaselo", c.nombre);
        else if (entregado) snprintf(linea, sizeof linea, u8"%s — entregado", c.nombre);
        else snprintf(linea, sizeof linea, "???");
        filaMarcada(L, fPeque, linea, entregado, entregado, tiene ? ROJO : (entregado ? TINTA_SUAVE : TINTA_SUAVE), alto);
    }
}

void hojaJefes(Lienzo& L, const Pagina& pg) {
    auto v = jefesVisibles();
    int muertos = 0, base = 0;
    for (int i : v) { if (!JEFES[i].dlc) ++base; if (!JEFES[i].dlc && F(JEFES[i].flag)) ++muertos; }
    char t[64]; snprintf(t, sizeof t, u8"Jefes derrotados (%d/%d)", muertos, base);
    titulo(L, t, pg.primera);
    bool dlcTitulo = false;
    for (int k = pg.desde; k < pg.hasta; ++k) {
        const JefeInfo& j = JEFES[v[k]];
        if (j.dlc && !dlcTitulo) {
            dlcTitulo = true;
            if (k == 0 || !JEFES[v[k - 1]].dlc) { L.espacio(4); L.texto(fCursiva, TINTA_SUAVE, u8"Contenido descargable", 4 * E); }
        }
        bool d = F(j.flag);
        float y0 = L.y;
        marca(L.dl, ImVec2(L.x + 4 * E, L.y + 9 * E), E, d, L.alfa);
        L.x = L.x0 + 28 * E;
        L.texto(fTexto, d ? TINTA : TINTA_SUAVE, d ? j.nombre : "???", 0);
        char sub[128];
        if (d) snprintf(sub, sizeof sub, u8"%s · derrotado", ZONAS[j.zona].nombre);
        else snprintf(sub, sizeof sub, u8"pendiente");   // la zona no se revela hasta vencerlo
        L.texto(fCursiva, d ? TINTA_SUAVE : IM_COL32(120, 92, 62, 255), sub, 8 * E);
        L.x = L.x0;
        (void)y0;
    }
}

void hojaNpcs(Lienzo& L, const Pagina& pg) {
    auto v = npcsVisibles();
    titulo(L, u8"Personas que conociste", pg.primera);
    if (v.empty()) { L.texto(fCursiva, TINTA_SUAVE, u8"Aún no has hablado con nadie. Cuando hables con alguien, aparecerá aquí con su retrato."); return; }
    for (int k = pg.desde; k < pg.hasta; ++k) {
        int i = v[k]; const Npc& n = NPCS[i];
        float lado_f = 128 * E, y0 = L.y;
        ImVec2 p(L.x0, L.y);
        ImDrawList* dl = L.dl;
        dl->AddRectFilled(ImVec2(p.x - 4 * E, p.y - 4 * E), ImVec2(p.x + lado_f + 4 * E, p.y + lado_f + 4 * E), conAlfa(IM_COL32(245, 236, 210, 255), L.alfa), 2);
        if (fotos.count(i) && fotos[i]) dl->AddImage((ImTextureID)fotos[i], p, ImVec2(p.x + lado_f, p.y + lado_f), ImVec2(0, 0), ImVec2(1, 1), conAlfa(IM_COL32_WHITE, L.alfa));
        else {
            dl->AddRectFilled(p, ImVec2(p.x + lado_f, p.y + lado_f), conAlfa(IM_COL32(200, 180, 140, 255), L.alfa));
            const char* sf = u8"sin retrato";
            float tw = fPeque->CalcTextSizeA(fPeque->FontSize, FLT_MAX, 0, sf).x;
            dl->AddText(fPeque, fPeque->FontSize, ImVec2(p.x + (lado_f - tw) / 2, p.y + lado_f / 2 - fPeque->FontSize / 2), conAlfa(TINTA_SUAVE, L.alfa), sf);
        }
        dl->AddRect(ImVec2(p.x - 4 * E, p.y - 4 * E), ImVec2(p.x + lado_f + 4 * E, p.y + lado_f + 4 * E), conAlfa(IM_COL32(110, 80, 50, 200), L.alfa), 2, 0, 1.5f * E);
        L.x = L.x0 + lado_f + 18 * E;
        L.texto(fTexto, TINTA, n.nombre, 3 * E);
        int idx = (int)(&n - NPCS), fase = faseActual(idx);
        const char* lugar = fase ? lugarDeFase(fase) : nullptr;
        std::string avanzar = fase ? paraAvanzar(idx, fase) : "";
        if (muertoNpc(idx)) L.texto(fPeque, ROJO, u8"Ha muerto. Su historia terminó.");
        else {
            if (lugar) { std::string t = std::string(u8"Ahora está en ") + lugar + "."; L.texto(fPeque, TINTA, t.c_str(), 2 * E); }
            if (!avanzar.empty()) { std::string t = u8"Para avanzar: " + avanzar + "."; t[14] = (char)toupper(t[14]); L.texto(fPeque, TINTA, t.c_str(), 2 * E); }
            else if (fase && !tieneSalida(fase) && profundidad(fase) > 0) L.texto(fPeque, TINTA_SUAVE, u8"Su historia parece haber llegado a su fin.", 2 * E);
            L.texto(fCursiva, TINTA_SUAVE, n.pasos[pasoActual(n)].texto, 4 * E);
            for (const Conexion& c : n.conexiones) {
                if (!c.conNombre) break;
                const char* msg = (c.otro < 0 || conocido(c.otro)) ? c.conNombre : c.anonimo;
                if (msg) L.texto(fPeque, ROJO, msg, 2 * E);
            }
        }
        L.x = L.x0;
        L.y = fmaxf(L.y, y0 + lado_f + 8 * E) + 6 * E;
        if (k + 1 < pg.hasta) L.separador();
    }
}

void hojaVacia(Lienzo&) {}   // página en blanco: solo lleva su número

// ---------------------------------------------------------------- libro: geometría y dibujo
struct Libro { ImVec2 izqA, izqB, derA, derB; float lomo, ancho; };

Libro geometria() {
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float alto = ds.y * 0.88f;
    float ancho = fminf(alto * 0.78f, ds.x * 0.45f);
    float cx = ds.x * 0.5f, top = (ds.y - alto) * 0.5f;
    return {ImVec2(cx - ancho, top), ImVec2(cx, top + alto), ImVec2(cx, top), ImVec2(cx + ancho, top + alto), cx, ancho};
}

// Papel de una hoja con sombra del lomo
void hoja(ImDrawList* dl, ImVec2 a, ImVec2 b, int lado, float alfa, float uvx0 = 0, float uvx1 = 1) {
    dl->AddImage((ImTextureID)texPapel, a, b, ImVec2(uvx0, 0), ImVec2(uvx1, 1), conAlfa(IM_COL32_WHITE, alfa));
    float s = 70 * E;
    ImU32 osc = conAlfa(IM_COL32(60, 35, 15, 110), alfa), nada = IM_COL32(60, 35, 15, 0);
    if (lado == 0) dl->AddRectFilledMultiColor(ImVec2(b.x - s, a.y), b, nada, osc, osc, nada);
    else dl->AddRectFilledMultiColor(a, ImVec2(a.x + s, b.y), osc, nada, nada, osc);
}

// Cuero de la tapa (textura del papel teñida)
void cuero(ImDrawList* dl, ImVec2 a, ImVec2 b, float alfa) {
    dl->AddRectFilled(ImVec2(a.x + 8 * E, a.y + 12 * E), ImVec2(b.x + 8 * E, b.y + 12 * E), conAlfa(IM_COL32(0, 0, 0, 120), alfa), 14 * E);
    dl->AddImageRounded((ImTextureID)texPapel, a, b, ImVec2(0.1f, 0.1f), ImVec2(0.6f, 0.9f), conAlfa(IM_COL32(120, 66, 34, 255), alfa), 12 * E);
    dl->AddRect(ImVec2(a.x + 10 * E, a.y + 10 * E), ImVec2(b.x - 10 * E, b.y - 10 * E), conAlfa(IM_COL32(175, 130, 70, 120), alfa), 8 * E, 0, 2 * E);
}

void contenido(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec4 clip, int vista, int lado, int zonaAct, float alfa) {
    if (vista <= 0) return;
    float m = 58 * E;
    Lienzo L{dl, a.x + m, a.x + m, a.y + 48 * E, (b.x - a.x) - 2 * m, alfa, clip};
    L.empezar();
    auto paginas = paginasLibro();
    int idx = (vista - 1) * 2 + lado;
    if (idx >= (int)paginas.size()) { L.terminar(); return; }
    const Pagina& pg = paginas[idx];
    switch (pg.tipo) {
        case P_ZONAS_IZQ: hojaZonasIzq(L, zonaAct); break;
        case P_ZONAS_DER: hojaZonasDer(L, zonaAct); break;
        case P_PRINCIPALES: hojaPrincipales(L); break;
        case P_SECUNDARIAS: hojaSecundarias(L, pg); break;
        case P_OBJETOS: hojaObjetos(L, pg); break;
        case P_COLECCIONABLES: hojaColeccionables(L, pg); break;
        case P_JEFES: hojaJefes(L, pg); break;
        case P_PERSONAS: hojaNpcs(L, pg); break;
        default: hojaVacia(L); break;
    }
    // número de página y ayuda
    int num = (vista - 1) * 2 + lado + 1;
    char t[32]; snprintf(t, sizeof t, u8"— %d —", num);
    float tw = fPeque->CalcTextSizeA(fPeque->FontSize, FLT_MAX, 0, t).x;
    dl->AddText(fPeque, fPeque->FontSize, ImVec2((a.x + b.x - tw) / 2, b.y - 46 * E), conAlfa(TINTA_SUAVE, alfa), t, nullptr, 0, &clip);
    L.terminar();
}

ImVec4 rect4(ImVec2 a, ImVec2 b) { return ImVec4(a.x, a.y, b.x, b.y); }

// Hoja que gira sobre el lomo. f en [0,1]: 0 = sobre la página derecha, 1 = sobre la izquierda.
// frente/dorso: qué contenido lleva la hoja en cada mitad del giro (-1 = cuero de la tapa).
void hojaGirando(ImDrawList* dl, const Libro& lb, float f, int vistaFrente, int vistaDorso, int zonaAct, float alfa) {
    float c = cosf(3.14159265f * f);
    float alto = lb.derB.y - lb.derA.y;
    float xLibre = lb.lomo + lb.ancho * c;
    float lift = sinf(3.14159265f * f) * alto * 0.035f;
    bool derecha = c >= 0;
    float x0 = derecha ? lb.lomo : xLibre, x1 = derecha ? xLibre : lb.lomo;
    if (x1 - x0 < 1) return;
    // sombra que la hoja proyecta sobre la página de debajo
    float sw = 90 * E * (1.f - fabsf(c) * 0.5f);
    ImU32 so = conAlfa(IM_COL32(30, 18, 8, 120), alfa), sn = IM_COL32(30, 18, 8, 0);
    if (derecha) dl->AddRectFilledMultiColor(ImVec2(xLibre, lb.derA.y), ImVec2(fminf(xLibre + sw, lb.derB.x), lb.derB.y), so, sn, sn, so);
    else dl->AddRectFilledMultiColor(ImVec2(fmaxf(xLibre - sw, lb.izqA.x), lb.izqA.y), ImVec2(xLibre, lb.izqB.y), sn, so, so, sn);
    // la hoja (cuadrilátero levantado en el borde libre)
    ImVec2 pLomoA(lb.lomo, lb.derA.y), pLomoB(lb.lomo, lb.derB.y), pLibA(xLibre, lb.derA.y - lift), pLibB(xLibre, lb.derB.y + lift * 0.4f);
    int v = derecha ? vistaFrente : vistaDorso;
    ImU32 tinte = v < 0 ? IM_COL32(120, 66, 34, 255) : IM_COL32_WHITE;
    if (derecha) dl->AddImageQuad((ImTextureID)texPapel, pLomoA, pLibA, pLibB, pLomoB, ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), conAlfa(tinte, alfa));
    else dl->AddImageQuad((ImTextureID)texPapel, pLibA, pLomoA, pLomoB, pLibB, ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), conAlfa(tinte, alfa));
    ImVec4 clip(x0, lb.derA.y, x1, lb.derB.y);
    if (v < 0) {   // tapa: título en dorado
        if (derecha && c > 0.55f) {
            ImFont* ft = fTitulo; const char* t1 = u8"Cuaderno del";
            const char* t2 = u8"Ceniciento";
            float cx = lb.lomo + (xLibre - lb.lomo) / 2, cy = (lb.derA.y + lb.derB.y) / 2;
            ImU32 oro = conAlfa(IM_COL32(214, 172, 96, 255), alfa * (c - 0.55f) / 0.45f);
            for (const char* t : {t1, t2}) {
                float tw = ft->CalcTextSizeA(ft->FontSize, FLT_MAX, 0, t).x * c;
                dl->AddText(ft, ft->FontSize, ImVec2(cx - tw / 2, cy - (t == t1 ? ft->FontSize * 1.1f : 0)), oro, t, nullptr, 0, &clip);
            }
            dl->AddRect(ImVec2(lb.lomo + 24 * E * c, lb.derA.y + 24 * E), ImVec2(xLibre - 24 * E * c, lb.derB.y - 24 * E), oro, 6 * E, 0, 2 * E);
        }
    } else {
        ImVec2 a = derecha ? lb.derA : lb.izqA, b = derecha ? lb.derB : lb.izqB;
        contenido(dl, a, b, clip, v, derecha ? 1 : 0, zonaAct, alfa);
    }
    // sombreado: la hoja se oscurece al ponerse vertical y cerca del pliegue
    float k = (1.f - fabsf(c)) * 0.55f;
    ImU32 o1 = conAlfa(IM_COL32(40, 22, 8, (int)(255 * k)), alfa), o2 = conAlfa(IM_COL32(40, 22, 8, (int)(255 * k * 0.35f)), alfa);
    if (derecha) dl->AddRectFilledMultiColor(ImVec2(x0, lb.derA.y), ImVec2(x1, lb.derB.y), o1, o2, o2, o1);
    else dl->AddRectFilledMultiColor(ImVec2(x0, lb.derA.y), ImVec2(x1, lb.derB.y), o2, o1, o1, o2);
}

// ---------------------------------------------------------------- animación
enum { ANIM_NADA, ANIM_ABRIR, ANIM_PAGINA, ANIM_CERRAR };
int anim = ANIM_NADA, vistaAnterior = 0;
double animInicio = 0;
constexpr float DUR_ABRIR = 0.75f, DUR_PAGINA = 0.55f, DUR_CERRAR = 0.28f;
float suaviza(float t) { t = fminf(1.f, fmaxf(0.f, t)); return t * t * (3 - 2 * t); }

// Ayuda de controles centrada bajo el libro
void ayudaControles(ImDrawList* dl, const Libro& lb, float alfa) {
    auto letra = [](int k) { return (k >= 'A' && k <= 'Z') ? (char)k : '?'; };
    char pie[128];
    if (usandoMando) snprintf(pie, sizeof pie, u8"L3 / R3: pasar página        L3 + R3: cerrar");
    else snprintf(pie, sizeof pie, u8"%c / %c: pasar página        %c: cerrar",
                  letra(teclaAnterior), letra(teclaSiguiente), letra(teclaCuaderno));
    float sz = fCursiva->FontSize * 0.85f;
    float tw = fCursiva->CalcTextSizeA(sz, FLT_MAX, 0, pie).x;
    ImVec2 p(lb.lomo - tw / 2, lb.derB.y + 22 * E);
    dl->AddText(fCursiva, sz, ImVec2(p.x + 1, p.y + 1), conAlfa(IM_COL32(0, 0, 0, 200), alfa), pie);
    dl->AddText(fCursiva, sz, p, conAlfa(IM_COL32(225, 205, 165, 230), alfa), pie);
}

void dibujarLibro(int zonaAct) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    Libro lb = geometria();
    float t = (float)(ImGui::GetTime() - animInicio);
    float dur = anim == ANIM_ABRIR ? DUR_ABRIR : anim == ANIM_PAGINA ? DUR_PAGINA : anim == ANIM_CERRAR ? DUR_CERRAR : 0;
    if (anim != ANIM_NADA && t >= dur) {
        bool eraCierre = anim == ANIM_CERRAR;
        anim = ANIM_NADA;
        if (eraCierre || vista == 0) return;   // el cierre terminó: no dibujar un libro vacío
    }
    if (anim == ANIM_NADA && vista == 0) return;

    float alfa = 1.f;
    if (anim == ANIM_ABRIR) alfa = suaviza(t / 0.18f);
    if (anim == ANIM_CERRAR) alfa = 1.f - suaviza(t / DUR_CERRAR);
    int v = anim == ANIM_CERRAR ? vistaAnterior : vista;

    bg->AddRectFilled(ImVec2(0, 0), ds, conAlfa(IM_COL32(8, 6, 4, 165), alfa));
    float mt = 16 * E;
    ImVec2 tapaA(lb.izqA.x - mt, lb.izqA.y - mt), tapaB(lb.derB.x + mt, lb.derB.y + mt);

    if (anim == ANIM_ABRIR) {
        float f = suaviza((t - 0.12f) / (DUR_ABRIR - 0.12f));
        float xLibre = lb.lomo + lb.ancho * cosf(3.14159265f * f);
        // tapa trasera: la mitad derecha siempre; la izquierda aparece bajo la hoja que gira
        cuero(bg, ImVec2(f >= 0.5f ? fminf(xLibre - mt, lb.lomo) : lb.lomo - 2 * E, tapaA.y), tapaB, alfa);
        hoja(bg, lb.derA, lb.derB, 1, alfa);
        contenido(bg, lb.derA, lb.derB, rect4(lb.derA, lb.derB), v, 1, zonaAct, alfa);
        hojaGirando(fg, lb, f, -1, v, zonaAct, alfa);
        return;
    }

    cuero(bg, tapaA, tapaB, alfa);
    // cantos de las hojas en los bordes exteriores
    for (int i = 1; i <= 3; ++i) {
        bg->AddLine(ImVec2(lb.izqA.x - i * 3 * E, lb.izqA.y + i * 2 * E), ImVec2(lb.izqA.x - i * 3 * E, lb.izqB.y - i * 2 * E), conAlfa(IM_COL32(200, 180, 140, 200), alfa), 1.5f * E);
        bg->AddLine(ImVec2(lb.derB.x + i * 3 * E, lb.derA.y + i * 2 * E), ImVec2(lb.derB.x + i * 3 * E, lb.derB.y - i * 2 * E), conAlfa(IM_COL32(200, 180, 140, 200), alfa), 1.5f * E);
    }
    hoja(bg, lb.izqA, lb.izqB, 0, alfa);
    hoja(bg, lb.derA, lb.derB, 1, alfa);

    if (anim == ANIM_PAGINA) {
        float f = suaviza(t / DUR_PAGINA);
        bool adelante = vista > vistaAnterior;
        int izqDebajo = adelante ? vistaAnterior : vista;     // lo que se ve bajo la hoja
        int derDebajo = adelante ? vista : vistaAnterior;
        contenido(bg, lb.izqA, lb.izqB, rect4(lb.izqA, lb.izqB), izqDebajo, 0, zonaAct, alfa);
        contenido(bg, lb.derA, lb.derB, rect4(lb.derA, lb.derB), derDebajo, 1, zonaAct, alfa);
        if (adelante) hojaGirando(fg, lb, f, vistaAnterior, vista, zonaAct, alfa);
        else hojaGirando(fg, lb, 1.f - f, vista, vistaAnterior, zonaAct, alfa);
        return;
    }
    contenido(bg, lb.izqA, lb.izqB, rect4(lb.izqA, lb.izqB), v, 0, zonaAct, alfa);
    contenido(bg, lb.derA, lb.derB, rect4(lb.derA, lb.derB), v, 1, zonaAct, alfa);
    ayudaControles(bg, lb, alfa);
}

void diagnosticoTick(int region, int dialogo, int linea) {
    if (!diagnostico) return;
    if (region != regionPrevia) {
        int z = zonaDeRegionCompleta(region);
        logf("Región %d -> %s%s", region, z >= 0 && z < Z_TOTAL ? ZONAS[z].nombre : "?", z == Z_DESCONOCIDA && region > 0 ? "  [REGIÓN SIN ASIGNAR]" : "");
        regionPrevia = region;
    }
    if (dialogo != dialogoPrevio) { if (dialogo > 0) logf("Diálogo %d (NPC %d)", dialogo, dialogo / 1000000); dialogoPrevio = dialogo; }
    if (linea != enLineaPrevio) { logf("Modo en línea: %d", linea); enLineaPrevio = linea; }
    ULONGLONG t = GetTickCount64();
    if (t - diagUltimo < 1000) return;
    diagUltimo = t;
    uint8_t act[sizeof diagPrevio];
    if (!g.bloqueFlags(DIAG_DESDE, DIAG_HASTA, act)) return;
    if (diagIniciado) {
        for (uint32_t i = 0; i < sizeof act; ++i) {
            if (act[i] == diagPrevio[i]) continue;
            for (int bit = 0; bit < 8; ++bit) {
                if (((act[i] ^ diagPrevio[i]) >> bit) & 1) {
                    uint32_t off = DIAG_DESDE + i, byte = off % 0x80;
                    uint32_t b = (byte / 4) * 32 + (3 - byte % 4) * 8 + (7 - bit);
                    uint32_t fid = (off / 0x80) * 1000 + b;
                    const char* quien = "";
                    for (int n = 0; n < N_TOTAL; ++n) { int bl = bloqueNpc(n); if (bl && fid >= (uint32_t)bl && fid < (uint32_t)bl + 20) quien = NPCS[n].nombre; }
                    logf("Flag %u -> %d (región %d)%s%s", fid, (act[i] >> bit) & 1, region, *quien ? " NPC: " : "", quien);
                }
            }
        }
    } else {
        // foto inicial de los flags 1000–1999 activos, útil para saber en qué estado está cada NPC
        std::string s;
        for (uint32_t i = 0; i < sizeof act; ++i)
            for (int bit = 0; bit < 8; ++bit)
                if ((act[i] >> bit) & 1) {
                    uint32_t off = DIAG_DESDE + i, byte = off % 0x80;
                    s += std::to_string((off / 0x80) * 1000 + (byte / 4) * 32 + (3 - byte % 4) * 8 + (7 - bit)) + " ";
                }
        logf("Flags 1000-1999 activos al cargar: %s", s.c_str());
        diagIniciado = true;
    }
    memcpy(diagPrevio, act, sizeof act);
}
} // namespace

// =================================================================== API
namespace cuaderno {
bool modoDemo() { return demo; }
int cuadroCaptura() { return demoCaptura; }

// Captura de pantalla completa a BMP (para el modo demostración / página del mod).
void guardarCaptura(IDXGISwapChain* sc, const char* ruta) {
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) return;
    D3D11_TEXTURE2D_DESC d; bb->GetDesc(&d);
    D3D11_TEXTURE2D_DESC sd = d; sd.Usage = D3D11_USAGE_STAGING; sd.BindFlags = 0; sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ; sd.MiscFlags = 0;
    ID3D11Texture2D* st = nullptr;
    if (FAILED(dev->CreateTexture2D(&sd, nullptr, &st))) { bb->Release(); return; }
    ctx->CopyResource(st, bb); bb->Release();
    D3D11_MAPPED_SUBRESOURCE m;
    if (SUCCEEDED(ctx->Map(st, 0, D3D11_MAP_READ, 0, &m))) {
        bool bgra = d.Format == DXGI_FORMAT_B8G8R8A8_UNORM || d.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        if (FILE* f = fopen(ruta, "wb")) {
            int W = d.Width, H = d.Height, fila = (W * 3 + 3) & ~3;
            BITMAPFILEHEADER fh{0x4D42, (DWORD)(54 + fila * H), 0, 0, 54};
            BITMAPINFOHEADER ih{sizeof ih, W, H, 1, 24, BI_RGB, (DWORD)(fila * H), 2835, 2835, 0, 0};
            fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f);
            std::vector<uint8_t> row(fila);
            for (int y = H - 1; y >= 0; --y) {
                const uint8_t* p = (const uint8_t*)m.pData + y * m.RowPitch;
                for (int x = 0; x < W; ++x) {
                    row[x * 3 + 0] = bgra ? p[x * 4 + 0] : p[x * 4 + 2];
                    row[x * 3 + 1] = p[x * 4 + 1];
                    row[x * 3 + 2] = bgra ? p[x * 4 + 2] : p[x * 4 + 0];
                }
                fwrite(row.data(), 1, fila, f);
            }
            fclose(f);
            logf("Captura guardada en %s", ruta);
        }
        ctx->Unmap(st, 0);
    }
    st->Release();
}

void iniciar(ID3D11Device* d, ID3D11DeviceContext* c, HWND h, const char* carpeta) {
    dev = d; ctx = c; hwnd = h; carpetaJuego = carpeta;
    carpetaMod = carpetaJuego + "\\CuadernoDS3";
    CreateDirectoryA(carpetaMod.c_str(), nullptr);
    cargarConfig();
    crearPapel();
    prepararSonidos();
    texIcono = crearTextura(128, 128, (const uint32_t*)icono_pergamino);
    if (demo) {
        g.demo = true; g.listo = true;
        // partida de ejemplo: tras Vordt, en el Asentamiento, con Anri, Sirris, Greirat y Yoel conocidos
        for (uint32_t f : std::initializer_list<uint32_t>{(uint32_t)J_IUDEX, (uint32_t)J_VORDT, 14000000u, 14000001u, 14000002u, 13000009u, 13000005u, 13000002u, 13100004u, 13100000u, 13100002u})
            g.demoFlags.insert(f);
        npcsConocidos = {N_SIRRIS, N_GREIRAT, N_YOEL, N_ANRI, N_YORSHKA, N_PINTORA};
        for (uint32_t id : {0x40000000u | 2137, 0x40000000u | 2102, 0x40000000u | 2010, 0x40000000u | 2118, 0x40000000u | 2108, 0x40000000u | 2127, 0x40000000u | 2132})
            objetosVistos.insert(id);
        objetosAhora = {0x40000000u | 2137, 0x40000000u | 2102, 0x40000000u | 2127, 0x40000000u | 2132, 0x40000000u | 2108};
        fotoPendienteNpc = N_SIRRIS; fotoCuando = GetTickCount64() + 300;   // prueba del filtro sepia
        sonar(true);
        vista = demoVista;
        if (demoPaso >= 0) vista = demoAccion == 2 ? demoVista + 1 : demoVista - 1;   // se llegará a demo_vista con animación
        logf("MODO DEMOSTRACIÓN activo");
        return;
    }
    g.iniciar();
    engancharDialogos();
}

void cargarFuentes(float altoPantalla) {
    E = (altoPantalla > 0 ? altoPantalla / 1080.f : 1.f) * escala;
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar rangos[] = {0x0020, 0x00FF, 0x2013, 0x2014, 0x2018, 0x201E, 0x2026, 0x2026, 0};
    ImFontConfig cfg; cfg.FontDataOwnedByAtlas = false; cfg.OversampleH = 2;
    fTitulo  = io.Fonts->AddFontFromMemoryTTF((void*)fuente_regular, sizeof fuente_regular, 46 * E, &cfg, rangos);
    fTexto   = io.Fonts->AddFontFromMemoryTTF((void*)fuente_regular, sizeof fuente_regular, 31 * E, &cfg, rangos);
    fPeque   = io.Fonts->AddFontFromMemoryTTF((void*)fuente_regular, sizeof fuente_regular, 25 * E, &cfg, rangos);
    fCursiva = io.Fonts->AddFontFromMemoryTTF((void*)fuente_cursiva, sizeof fuente_cursiva, 28 * E, &cfg, rangos);
    fHud     = io.Fonts->AddFontFromMemoryTTF((void*)fuente_regular, sizeof fuente_regular, 18 * escala, &cfg, rangos);
    io.FontDefault = fTexto;
}

void antesDeDibujar(IDXGISwapChain* sc) {
    if (modoEnLinea) {
        // en línea el mod no hace nada: cancelar cualquier foto y devolver el juego a como estaba
        if (ses.npc >= 0) { ocultarJugador(false); ocultarInterfaz(false); ses.npc = -1; logf("Foto cancelada: el juego está en línea"); }
        fotoPendienteNpc = -1;
        return;
    }
    if (fotoPendienteNpc >= 0 && GetTickCount64() >= fotoCuando) { iniciarSesionFoto(fotoPendienteNpc); fotoPendienteNpc = -1; }
    avanzarSesionFoto(sc);
}

void dibujar() {
    if (!g.listo || !g.jugadorCargado()) { if (vista) vista = 0; guardarEstado(); return; }

    std::wstring nom = g.nombrePersonaje();
    int ran = g.ranura();
    static int ranuraActual = -2;
    if (!demo && !nom.empty() && (nom != personaje || ran != ranuraActual)) { cambiarPartida(nom, ran); ranuraActual = ran; }

    int linea = g.enLinea();
    modoEnLinea = linea == 1;
    if (modoEnLinea) {
        // en línea: solo el aviso, sin leer ni registrar nada más
        static bool avisado = false;
        if (!avisado) { logf("Juego en línea: el cuaderno no hace nada hasta jugar sin conexión"); avisado = true; }
        if (vista) { vista = 0; anim = ANIM_NADA; }
        ocultarSticks = false;
        float s = escala;
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        if (mostrarIcono) icono(fg, ImVec2(ds.x - 58 * s, 22 * s), s, true);
        const char* av = u8"Cuaderno desactivado: inicia el juego sin conexión";
        ImVec2 z = fHud->CalcTextSizeA(fHud->FontSize, FLT_MAX, 0, av);
        fg->AddText(fHud, fHud->FontSize, ImVec2(ds.x - 66 * s - z.x + 1, 34 * s + 1), IM_COL32(0, 0, 0, 200), av);
        fg->AddText(fHud, fHud->FontSize, ImVec2(ds.x - 66 * s - z.x, 34 * s), IM_COL32(240, 200, 170, 255), av);
        return;
    }
    int region = g.regionActual(), dialogo = demo ? -1 : dialogoReciente();
    diagnosticoTick(region, dialogo, linea);

    if (!demo) actualizarInventario();
    int zonaAct = zonaDeRegionCompleta(region);
    if (zonaAct >= 0 && zonaAct < Z_TOTAL && !zonasVisitadas.count(zonaAct)) { zonasVisitadas.insert(zonaAct); estadoSucio = true; }

    // ¿con quién hablas? (TalkID / 1000000 = NPC)
    int npcHablando = -1;
    if (dialogo > 0)
        for (int i = 0; i < N_TOTAL; ++i) {
            int pre = dialogo / 1000000;
            if ((NPCS[i].prefijoDialogo && pre == NPCS[i].prefijoDialogo) || (prefijo2(i) && pre == prefijo2(i))) npcHablando = i;
        }
    if (npcHablando >= 0 && npcHablando != ultimoDialogoNpc) {
        if (!npcsConocidos.count(npcHablando)) { npcsConocidos.insert(npcHablando); estadoSucio = true; logf("Conociste a %s", NPCS[npcHablando].nombre); }
        if (!fotos.count(npcHablando) || !fotos[npcHablando]) { fotoPendienteNpc = npcHablando; fotoCuando = GetTickCount64() + 600; }
    }
    ultimoDialogoNpc = npcHablando;
    if (npcHablando >= 0 && teclaPulsada(teclaFoto)) { fotoPendienteNpc = npcHablando; fotoCuando = GetTickCount64() + 150; }
    // Horace no habla: lo das por conocido cuando está en las Catacumbas/Lago o en el Santuario con Anri
    if (!npcsConocidos.count(N_HORACE) && npcsConocidos.count(N_ANRI) && FB(NPCS[N_HORACE].enSantuario)) { npcsConocidos.insert(N_HORACE); estadoSucio = true; }

    static ULONGLONG ultimoGuardado = 0;
    if (GetTickCount64() - ultimoGuardado > 10000) { guardarEstado(); ultimoGuardado = GetTickCount64(); }

    // entrada (durante una animación se ignora):
    //   teclado: K abre/cierra, L página siguiente, J página anterior
    //   mando:   L3+R3 recorre abrir -> páginas -> cerrar
    bool bloqueado = linea == 1;
    bool pCuaderno = teclaPulsada(teclaCuaderno), pSig = teclaPulsada(teclaSiguiente), pAnt = teclaPulsada(teclaAnterior);
    prepararMando();
    detectarDispositivo();
    int aMando = npcHablando < 0 ? accionMando() : 0;
    bool pMando = false;
    if (aMando == 1) pCuaderno = true;
    else if (aMando == 2 && vista > 0) pSig = true;
    else if (aMando == 3 && vista > 0) pAnt = true;
    if (demoPaso >= 0 && ++demoCuadro == demoPaso) { if (demoAccion == 0) pCuaderno = true; else if (demoAccion == 1) pSig = true; else pAnt = true; }
    int ultima = totalPliegos();
    if (vista > ultima) vista = ultima;
    if (anim == ANIM_NADA && (pCuaderno || pSig || pAnt || pMando)) {
        int nueva = vista;
        if (bloqueado) nueva = 0;
        else if (pCuaderno) nueva = vista == 0 ? 1 : 0;
        else if (vista > 0 && pSig && vista < ultima) nueva = vista + 1;
        else if (vista > 0 && pAnt && vista > 1) nueva = vista - 1;
        if (nueva != vista) {
            vistaAnterior = vista;
            vista = nueva;
            animInicio = ImGui::GetTime();
            if (vistaAnterior == 0)  { anim = ANIM_ABRIR;  sonar(true); }
            else if (vista == 0)     { anim = ANIM_CERRAR; }
            else                     { anim = ANIM_PAGINA; sonar(false); }
        }
    }

    ocultarSticks = vista != 0 || anim != ANIM_NADA;
    float s = escala;
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    if (bloqueado) {
        if (mostrarIcono) icono(fg, ImVec2(ds.x - 58 * s, 22 * s), s, true);
        const char* av = u8"Cuaderno desactivado: inicia el juego sin conexión";
        ImVec2 z = fHud->CalcTextSizeA(fHud->FontSize, FLT_MAX, 0, av);
        fg->AddText(fHud, fHud->FontSize, ImVec2(ds.x - 66 * s - z.x + 1, 34 * s + 1), IM_COL32(0, 0, 0, 200), av);
        fg->AddText(fHud, fHud->FontSize, ImVec2(ds.x - 66 * s - z.x, 34 * s), IM_COL32(240, 200, 170, 255), av);
        return;
    }
    if (vista == 0 && anim != ANIM_CERRAR) {
        if (mostrarIcono) icono(fg, ImVec2(ds.x - 58 * s, 22 * s), s, false);
        return;
    }
    dibujarLibro(zonaAct);
}

} // namespace cuaderno
