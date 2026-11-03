// Cuaderno DS3 — interfaz del cuaderno.  © 2026 Angel.
#pragma once
#include <d3d11.h>
#include "juego.h"

namespace cuaderno {
void iniciar(ID3D11Device* dev, ID3D11DeviceContext* ctx, HWND hwnd, const char* carpeta);
void antesDeDibujar(IDXGISwapChain* sc);   // captura de fotos (antes del overlay)
void dibujar();                             // dentro de ImGui::NewFrame/Render
void cargarFuentes(float altoPantalla);      // antes del primer frame
int cuadroCaptura();
bool modoDemo();
void guardarCaptura(IDXGISwapChain* sc, const char* ruta);
}
