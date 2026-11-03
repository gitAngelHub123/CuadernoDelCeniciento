// Programa de prueba: imita la ventana de DS3 (DX11) y carga dinput8.dll en modo demostración.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <cstdint>
#include <cmath>
static LRESULT CALLBACK wp(HWND h, UINT m, WPARAM w, LPARAM l) { if (m == WM_DESTROY) PostQuitMessage(0); return DefWindowProcW(h, m, w, l); }
int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    const int W = 1920, H = 1080;
    WNDCLASSW wc{0, wp, 0, 0, hi, nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"PruebaDS3"};
    RegisterClassW(&wc);
    RECT r{0, 0, W, H}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hw = CreateWindowW(L"PruebaDS3", L"DARK SOULS III", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, r.right - r.left, r.bottom - r.top, nullptr, nullptr, hi, nullptr);
    char dir[MAX_PATH]; GetModuleFileNameA(nullptr, dir, MAX_PATH); *strrchr(dir, '\\') = 0;
    char dll[MAX_PATH]; wsprintfA(dll, "%s\\dinput8.dll", dir);
    HMODULE m = LoadLibraryA(dll);
    if (!m) return 2;
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Width = W; sd.BufferDesc.Height = H; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hw; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    IDXGISwapChain* sc; ID3D11Device* dev; ID3D11DeviceContext* ctx;
    Sleep(1500);   // dar tiempo a que el mod se enganche (espera la ventana)
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx))) return 3;
    // fondo: degradado oscuro con "niebla", parecido a una escena del juego
    std::vector<uint32_t> px(W * H);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        float u = x / (float)W, v = y / (float)H;
        float n = 0.5f + 0.5f * sinf(u * 9 + sinf(v * 7) * 2) * cosf(v * 5 + u * 3);
        int R = (int)(30 + 40 * v + 18 * n), G = (int)(34 + 38 * v + 16 * n), B = (int)(42 + 30 * v + 14 * n);
        if (v > 0.72f) { R = (int)(R * 0.7f); G = (int)(G * 0.68f); B = (int)(B * 0.6f); }
        px[y * W + x] = 0xFF000000u | (B << 16) | (G << 8) | R;
    }
    D3D11_TEXTURE2D_DESC td{}; td.Width = W; td.Height = H; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA srd{px.data(), W * 4, 0};
    ID3D11Texture2D* fondo; dev->CreateTexture2D(&td, &srd, &fondo);
    for (int f = 0; f < 200; ++f) {
        MSG msg; while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        ID3D11Texture2D* bb; sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
        ctx->CopyResource(bb, fondo); bb->Release();
        sc->Present(1, 0);
    }
    return 0;
}
