// Exercita os registradores reais do SH7604; nenhum módulo de jogo é carregado.
#include "saturn.h"
#include "test.h"
#include <cstdint>

namespace {
struct CpuScope {
    SH2Context context{};
    SH2Context* previous = g_cpu;
    explicit CpuScope(int cpu = 0) {
        context.cpu = cpu;
        g_cpu = &context;
        onchip_reset(cpu);
    }
    ~CpuScope() { g_cpu = previous; }
    void write(uint32_t offset, uint32_t value) { onchip_write(context, 0xFFFFFE00u + offset, value, 4); }
    uint32_t read(uint32_t offset) { return onchip_read(context, 0xFFFFFE00u + offset, 4); }
};
}

TEST(onchip_dma_transfers_the_whole_count_for_each_unit_and_channel) {
    constexpr uint32_t source = 0x06020000u, destination = 0x06021000u;
    constexpr uint32_t counts[] = {4, 8, 16, 64};
    for (int cpu = 0; cpu < 2; ++cpu) for (int channel = 0; channel < 2; ++channel)
        for (uint32_t ts = 0; ts < 4; ++ts) for (uint32_t count : counts) {
            CpuScope scope(cpu);
            const uint32_t bytes = ts == 3 ? count * 4 : count * (1u << ts);
            for (uint32_t i = 0; i < 272; ++i) {
                st8(source + i, (i * 37 + 19) & 0xFF);
                st8(destination + i, 0xEE);
            }
            const uint32_t base = 0x180u + channel * 0x10u;
            scope.write(base, source);
            scope.write(base + 4, destination);
            scope.write(base + 8, count);
            // Endereço crescente, requisição automática, habilitado; sem IRQ.
            scope.write(base + 12, 1u | (1u << 9) | (ts << 10) | (1u << 12) | (1u << 14));
            scope.write(0x1B0, 1);
            for (uint32_t i = 0; i < bytes; ++i) CHECK_EQ(ld8(destination + i), (i * 37 + 19) & 0xFF);
            CHECK_EQ(ld8(destination + bytes), 0xEE);
            CHECK_EQ(scope.read(base), source + bytes);
            CHECK_EQ(scope.read(base + 4), destination + bytes);
            CHECK_EQ(scope.read(base + 8), 0);
            CHECK_EQ(scope.read(base + 12) & 2, 2);
        }
}

TEST(onchip_division_handles_host_overflow_without_a_native_exception) {
    for (int cpu = 0; cpu < 2; ++cpu) {
        CpuScope scope(cpu);
        scope.write(0x100, 0xFFFFFFFFu);
        scope.write(0x110, 0x80000000u);
        scope.write(0x114, 0);
        CHECK_EQ(scope.read(0x108) & 1, 1);
        CHECK_EQ(scope.read(0x104), 0x7FFFFFFFu);
        CHECK_EQ(scope.read(0x114), 0x7FFFFFFFu);
        CHECK_EQ(scope.read(0x124), 0x7FFFFFFFu);
        CHECK_EQ(scope.read(0x128) & 1, 1);
    }
}

TEST(onchip_division_preserves_normal_signed_results_and_overflow_signs) {
    struct Row { uint32_t high, low, divisor, quotient, remainder, overflow; };
    constexpr Row rows[] = {
        {0, 100, 7, 14, 2, 0},
        {0xFFFFFFFFu, uint32_t(-100), 7, uint32_t(-14), uint32_t(-2), 0},
        {0, 100, uint32_t(-7), uint32_t(-14), 2, 0},
        {0xFFFFFFFFu, uint32_t(-100), uint32_t(-7), 14, uint32_t(-2), 0},
        {0, 0x7FFFFFFFu, 1, 0x7FFFFFFFu, 0, 0},
        {0xFFFFFFFFu, 0x80000000u, 1, 0x80000000u, 0, 0},
        {0x80000000u, 0, 1, 0x80000000u, 0, 1},
        {0x7FFFFFFFu, 0xFFFFFFFFu, 1, 0x7FFFFFFFu, 0, 1},
        {0, 100, 0, 0x7FFFFFFFu, 0, 1},
        {0xFFFFFFFFu, uint32_t(-100), 0, 0x80000000u, 0, 1},
    };
    for (const Row& row : rows) {
        CpuScope scope;
        scope.write(0x100, row.divisor);
        scope.write(0x110, row.high);
        scope.write(0x114, row.low);
        CHECK_EQ(scope.read(0x114), row.quotient);
        CHECK_EQ(scope.read(0x110), row.remainder);
        CHECK_EQ(scope.read(0x108) & 1, row.overflow);
    }
    CpuScope scope;
    scope.write(0x120, 7);
    scope.write(0x124, uint32_t(-100));
    CHECK_EQ(scope.read(0x104), uint32_t(-14));
    CHECK_EQ(scope.read(0x110), uint32_t(-2));
    CHECK_EQ(scope.read(0x108) & 1, 0);
}
