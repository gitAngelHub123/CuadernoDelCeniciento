// Prueba en Linux de Juego::idAOffset (misma lógica que src/juego.cpp).
#include <cstdio>
#include <cstdint>
static int bloqueDe(int area, int d10000) {
    struct B { int a, d, b; };
    static const B t[] = {{30,0,3},{30,1,4},{31,0,5},{32,0,7},{33,0,9},{34,1,11},{35,0,12},{37,0,15},
                          {38,0,16},{39,0,17},{40,0,18},{41,0,19},{45,0,20},{50,0,23},{51,0,24},{51,1,25}};
    for (auto& x : t) if (x.a == area && x.d == d10000) return x.b;
    return -1;
}
static bool idAOffset(uint32_t id, uint32_t& off, uint8_t& bit) {
    int chunk;
    if (id < 10000) chunk = id / 1000;
    else { int bl = bloqueDe((id / 100000) % 100, (id / 10000) % 10); if (bl < 0 || id / 10000000 != 1) return false; chunk = bl * 10 + (id / 1000) % 10; }
    uint32_t b = id % 1000;
    off = chunk * 0x80 + (b >> 5) * 4 + (3 - ((b & 31) >> 3)); bit = 7 - (b & 7);
    return true;
}
int main() { unsigned id; while (scanf("%u", &id) == 1) { uint32_t o; uint8_t b; if (idAOffset(id, o, b)) printf("%u 0x%X.%u\n", id, o, b); else printf("%u NO\n", id); } }
