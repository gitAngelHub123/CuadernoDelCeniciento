#include "sonido.h"
#include <cstdio>
int main() {
    const char* n[] = {"apertura", "pagina1", "pagina2", "pagina3"};
    for (int k = 0; k < 4; ++k) {
        auto w = sonido::paginaWav(k == 0 ? 0 : k - 1, k == 0, 0.8f);
        char r[128]; snprintf(r, sizeof r, "capturas/sonidos/%s.wav", n[k]);
        FILE* f = fopen(r, "wb"); fwrite(w.data(), 1, w.size(), f); fclose(f);
        printf("%s %zu bytes\n", r, w.size());
    }
}
