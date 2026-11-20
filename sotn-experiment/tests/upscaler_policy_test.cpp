#include "sotn_video.h"
#include <cstdio>

int main() {
    // FSR ativo ou indisponível não deve apagar a preferência HD para o próximo quadro.
    for (int factor : SotnVideo::kHdFactors) {
        for (int active : SotnVideo::kHdFactors) {
            for (bool original : {false, true}) {
                if (SotnVideo::hd_failed(true, factor, active, original)) return 1;
                const bool expected = factor > 1 && (active != factor || original);
                if (SotnVideo::hd_failed(false, factor, active, original) != expected) return 2;
            }
        }
    }
    // Trocar FSR -> HD mantém a escolha anterior e só desativa após falha real de HD.
    const int selected = 8;
    if (SotnVideo::hd_failed(true, selected, 1, false)) return 3;
    if (SotnVideo::hd_failed(false, selected, selected, false)) return 4;
    if (!SotnVideo::hd_failed(false, selected, 1, true)) return 5;
    // O override orienta apenas Auto; escolhas explícitas e D3D11 continuam iguais.
    if (SotnVideo::gpu_backend(0, "vulkan") != 3) return 6;
    if (SotnVideo::gpu_backend(0, "direct3d12") != 1) return 7;
    if (SotnVideo::gpu_backend(0, "") != 0 || SotnVideo::gpu_backend(0, "invalid") != 0) return 8;
    for (int configured : {1, 2, 3}) {
        for (const char* preferred : {"", "vulkan", "direct3d12", "invalid"}) {
            if (SotnVideo::gpu_backend(configured, preferred) != configured) return 9;
        }
    }
    std::puts("PASS: FSR priority preserves HD preferences; genuine HD failure still disables it");
}
