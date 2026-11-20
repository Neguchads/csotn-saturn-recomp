#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Snapshot gráfico imutável. Nenhum ponteiro para a memória do jogo chega à GPU.
struct Vdp2GpuSnapshot {
    std::array<std::uint32_t, 132> params{};
    std::vector<std::uint8_t> bytes;
};
bool vdp2_gpu_snapshot(Vdp2GpuSnapshot& snapshot, const char** fallback_reason = nullptr);
