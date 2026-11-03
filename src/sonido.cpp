// Cuaderno DS3 — síntesis del sonido de pasar página.  © 2026 Angel.
// Roce de papel (ruido filtrado con barrido y aleteo) + crujidos + golpe suave al caer la hoja.
#include "sonido.h"
#include <cmath>
#include <cstring>

namespace {
constexpr int FS = 44100;
constexpr float PI = 3.14159265f;

struct Azar {
    uint32_t s;
    float operator()() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s & 0xFFFFFF) / 8388608.f - 1.f; }  // [-1,1)
};

// Biquad (RBJ) paso banda / paso alto / paso bajo
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void set(int tipo, float f, float q) {
        float w = 2 * PI * f / FS, c = cosf(w), al = sinf(w) / (2 * q), a0 = 1 + al;
        if (tipo == 0) { b0 = al; b1 = 0; b2 = -al; }                          // banda
        else if (tipo == 1) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; } // alto
        else { b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; }                // bajo
        b0 /= a0; b1 /= a0; b2 /= a0; a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
    float operator()(float x) {
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y; return y;
    }
};

float suave(float t) { return t <= 0 ? 0 : t >= 1 ? 1 : t * t * (3 - 2 * t); }
} // namespace

namespace sonido {

std::vector<uint8_t> paginaWav(int variante, bool apertura, float volumen) {
    Azar rnd{0x9E3779B9u * (uint32_t)(variante + 1) + (apertura ? 77 : 0)};
    const float dur = apertura ? 0.62f : 0.48f;
    const int N = (int)(dur * FS);
    std::vector<float> s(N, 0.f);

    // 1) roce: ruido en banda con centro que sube y baja (la hoja se curva y pasa)
    Biquad bp, hp, lfoF;
    hp.set(1, 700, 0.7f);
    lfoF.set(2, 28 + 10 * rnd(), 0.7f);            // aleteo lento del papel
    const float tRoce = dur * 0.78f;
    for (int i = 0; i < N; ++i) {
        float t = i / (float)FS, u = t / tRoce;
        if (u > 1) break;
        float centro = (apertura ? 1500 : 1900) + 2600 * sinf(PI * u) * (0.85f + 0.15f * rnd());
        if (i % 64 == 0) bp.set(0, centro, 0.9f);
        float env = suave(u / 0.18f) * (1 - suave((u - 0.55f) / 0.45f));
        float aleteo = 0.55f + 0.45f * fabsf(lfoF(rnd()) * 9.f);
        s[i] += hp(bp(rnd())) * env * fminf(aleteo, 1.3f) * 0.9f;
    }

    // 2) crujidos: impulsos cortos de ruido agudo repartidos durante el roce
    int nCruj = 14 + (int)(8 * (rnd() + 1));
    for (int k = 0; k < nCruj; ++k) {
        float t0 = tRoce * (0.08f + 0.85f * (rnd() + 1) / 2);
        float amp = 0.15f + 0.35f * (rnd() + 1) / 2;
        int largo = (int)(FS * (0.002f + 0.004f * (rnd() + 1) / 2));
        Biquad h; h.set(1, 2500 + 2000 * (rnd() + 1) / 2, 0.8f);
        int i0 = (int)(t0 * FS);
        for (int j = 0; j < largo && i0 + j < N; ++j)
            s[i0 + j] += h(rnd()) * amp * expf(-5.f * j / largo);
    }

    // 3) la hoja cae: golpe suave grave
    {
        float t0 = tRoce * 0.92f;
        Biquad lp; lp.set(2, 260, 0.8f);
        int i0 = (int)(t0 * FS), largo = (int)(FS * 0.07f);
        for (int j = 0; j < largo && i0 + j < N; ++j) {
            float e = (j < 40 ? j / 40.f : 1.f) * expf(-6.f * j / largo);
            s[i0 + j] += lp(rnd()) * e * 2.2f;
        }
    }

    // normalizar, aplicar volumen y fundidos
    float pico = 1e-6f;
    for (float v : s) pico = fmaxf(pico, fabsf(v));
    float g = 0.7f * volumen / pico;
    std::vector<uint8_t> wav(44 + N * 2);
    uint8_t* d = wav.data();
    auto u32 = [&](int o, uint32_t v) { memcpy(d + o, &v, 4); };
    auto u16 = [&](int o, uint16_t v) { memcpy(d + o, &v, 2); };
    memcpy(d, "RIFF", 4); u32(4, 36 + N * 2); memcpy(d + 8, "WAVEfmt ", 8);
    u32(16, 16); u16(20, 1); u16(22, 1); u32(24, FS); u32(28, FS * 2); u16(32, 2); u16(34, 16);
    memcpy(d + 36, "data", 4); u32(40, N * 2);
    for (int i = 0; i < N; ++i) {
        float f = fminf(1.f, fminf(i / 60.f, (N - i) / 400.f));
        float v = fmaxf(-1.f, fminf(1.f, s[i] * g * f));
        int16_t m = (int16_t)(v * 32767);
        memcpy(d + 44 + i * 2, &m, 2);
    }
    return wav;
}

} // namespace sonido
