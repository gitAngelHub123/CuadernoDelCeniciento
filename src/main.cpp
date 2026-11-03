// Cuaderno DS3 — proxy de dinput8 + overlay DX11 (cuaderno de orientación).
// © 2026 Angel. Usa Dear ImGui (MIT) y MinHook (BSD-2).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstdarg>
#include <atomic>
#include <initializer_list>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "MinHook.h"
#include "cuaderno.h"

// ---------------------------------------------------------------- registro
static char g_dir[MAX_PATH];   // carpeta del juego (donde está nuestro dll)
void logf(const char* fmt, ...) {
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s\\CuadernoDS3.log", g_dir);
    FILE* f = fopen(path, "a");
    if (!f) return;
    SYSTEMTIME t; GetLocalTime(&t);
    fprintf(f, "[%02d:%02d:%02d] ", t.wHour, t.wMinute, t.wSecond);
    va_list a; va_start(a, fmt); vfprintf(f, fmt, a); va_end(a);
    fputc('\n', f);
    fclose(f);
}

// ---------------------------------------------------------- proxy dinput8
using DI8Create_t = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
static DI8Create_t g_realDI8 = nullptr;

static void loadRealDinput8() {
    char sys[MAX_PATH];
    GetSystemDirectoryA(sys, MAX_PATH);
    strcat_s(sys, "\\dinput8.dll");
    HMODULE m = LoadLibraryA(sys);
    if (m) g_realDI8 = (DI8Create_t)GetProcAddress(m, "DirectInput8Create");
    logf("dinput8 real: %s -> %s", sys, g_realDI8 ? "ok" : "FALLO");
}

extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(
    HINSTANCE h, DWORD v, REFIID r, LPVOID* o, LPUNKNOWN u) {
    if (!g_realDI8) loadRealDinput8();
    return g_realDI8 ? g_realDI8(h, v, r, o, u) : E_FAIL;
}

// ------------------------------------------------------------ overlay DX11
using Present_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using Resize_t  = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static Present_t g_oPresent = nullptr;
static Resize_t  g_oResize  = nullptr;

static ID3D11Device*           g_dev = nullptr;
static ID3D11DeviceContext*    g_ctx = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND                    g_hwnd = nullptr;
static bool                    g_ready = false;
static std::atomic<long>       g_frames{0};

static void createRTV(IDXGISwapChain* sc) {
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) && bb) {
        g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
        bb->Release();
    }
}

static bool initImGui(IDXGISwapChain* sc) {
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_dev))) return false;
    g_dev->GetImmediateContext(&g_ctx);
    DXGI_SWAP_CHAIN_DESC d; sc->GetDesc(&d);
    g_hwnd = d.OutputWindow;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // no crear imgui.ini en la carpeta del juego
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(g_hwnd);
    cuaderno::iniciar(g_dev, g_ctx, g_hwnd, g_dir);
    cuaderno::cargarFuentes((float)d.BufferDesc.Height);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    createRTV(sc);
    logf("ImGui listo: ventana %p, %ux%u", (void*)g_hwnd, d.BufferDesc.Width, d.BufferDesc.Height);
    return true;
}

static HRESULT STDMETHODCALLTYPE hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!g_ready) g_ready = initImGui(sc);
    if (g_ready) {
        if (!g_rtv) createRTV(sc);
        cuaderno::antesDeDibujar(sc);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        cuaderno::dibujar();
        ImGui::Render();
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (cuaderno::modoDemo() && g_frames == cuaderno::cuadroCaptura()) {
            char ruta[MAX_PATH]; snprintf(ruta, sizeof ruta, "%s\\CuadernoDS3\\captura_demo.bmp", g_dir);
            cuaderno::guardarCaptura(sc, ruta);
        }
        if (++g_frames == 300) logf("300 cuadros dibujados sin problemas");
    }
    return g_oPresent(sc, sync, flags);
}

static HRESULT STDMETHODCALLTYPE hkResize(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT fmt, UINT fl) {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    HRESULT r = g_oResize(sc, n, w, h, fmt, fl);
    logf("ResizeBuffers %ux%u -> 0x%08lx", w, h, (unsigned long)r);
    return r;
}

// Crea un dispositivo temporal solo para leer las direcciones de Present/ResizeBuffers.
static bool hookDX11() {
    WNDCLASSEXA wc{sizeof wc, CS_CLASSDC, DefWindowProcA, 0, 0, GetModuleHandleA(nullptr),
                   nullptr, nullptr, nullptr, nullptr, "CuadernoDS3Tmp", nullptr};
    RegisterClassExA(&wc);
    HWND w = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                           nullptr, nullptr, wc.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = w;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    IDXGISwapChain* sc = nullptr; ID3D11Device* dev = nullptr; ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                               nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx);
    if (FAILED(hr)) {
        logf("D3D11CreateDeviceAndSwapChain falló: 0x%08lx", (unsigned long)hr);
        DestroyWindow(w); UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return false;
    }
    void** vt = *(void***)sc;
    void* pPresent = vt[8];
    void* pResize  = vt[13];
    sc->Release(); dev->Release(); ctx->Release();
    DestroyWindow(w); UnregisterClassA(wc.lpszClassName, wc.hInstance);

    if (MH_Initialize() != MH_OK) { logf("MH_Initialize falló"); return false; }
    bool ok = MH_CreateHook(pPresent, (void*)hkPresent, (void**)&g_oPresent) == MH_OK &&
              MH_CreateHook(pResize,  (void*)hkResize,  (void**)&g_oResize)  == MH_OK &&
              MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
    logf("Ganchos DX11 (Present %p, Resize %p): %s", pPresent, pResize, ok ? "ok" : "FALLO");
    return ok;
}

static DWORD WINAPI mainThread(LPVOID) {
    // Esperar a que el juego cree su ventana antes de engancharse.
    for (int i = 0; i < 600 && !FindWindowA(nullptr, "DARK SOULS III"); ++i) Sleep(100);
    logf("Ventana del juego %s", FindWindowA(nullptr, "DARK SOULS III") ? "encontrada" : "no encontrada (sigo igual)");
    hookDX11();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        GetModuleFileNameA(h, g_dir, MAX_PATH);
        if (char* s = strrchr(g_dir, '\\')) *s = 0;
        logf("==== Cuaderno del Ceniciento v1.0.0 cargado ====");
        CreateThread(nullptr, 0, mainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
