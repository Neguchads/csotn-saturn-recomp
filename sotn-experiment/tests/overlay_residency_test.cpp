#include "saturn/sh2.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr uint32_t header = 0x060DC000, base = header + 0x40, entry = base + 0x20;
int calls_a = 0, calls_b = 0;
void a(SH2Context&) { ++calls_a; }
void b(SH2Context&) { ++calls_b; }
const SH2FuncEntry entries_a[] = {{entry, a}}, entries_b[] = {{entry, b}};
SH2Module ma{"before", base, 128, 0, entries_a, 1, nullptr, nullptr, true};
SH2Module mb{"after", base, 128, 0, entries_b, 1, nullptr, nullptr, true};
std::array<uint8_t, 192> before{}, after{};
struct Unrecognized {};
}

extern const SH2Module* const g_sh2_modules[] = {&ma, &mb};
extern const int g_sh2_nmodules = 2;
void sh2_poll(SH2Context&) { throw std::runtime_error("unexpected poll"); }
void sh2_sleep(SH2Context&, uint32_t) { throw std::runtime_error("unexpected sleep"); }
void sh2_trapa(SH2Context&, uint32_t, uint32_t) { throw std::runtime_error("unexpected trap"); }
void sh2_bad_return(SH2Context&, uint32_t) { throw std::runtime_error("unexpected return"); }
void sh2_call_unknown(SH2Context&, uint32_t) { throw std::runtime_error("unexpected unknown call"); }
void sh2_program_start(SH2Context&, uint32_t) { throw std::runtime_error("unexpected program restart"); }
uint32_t sh2_mmio_read(uint32_t, int) { throw std::runtime_error("unexpected MMIO"); }
void sh2_mmio_write(uint32_t, uint32_t, int) { throw std::runtime_error("unexpected MMIO"); }
[[noreturn]] void sh2_bad_identity(SH2Context&, uint32_t, uint32_t) { throw Unrecognized{}; }

int main(int argc, char** argv) {
    for (size_t i = 0; i < before.size(); ++i) {
        before[i] = uint8_t(i * 7 + 3);
        after[i] = uint8_t(i * 11 + 1);
    }
    ma.crc = sh2_crc32(before.data() + 64, 128);
    mb.crc = sh2_crc32(after.data() + 64, 128);
    sh2_watch_residency(header, 64, base, 0x06100000);
    assert(sh2_mem_write(header, before.data(), before.size()));
    sh2_activate(&ma);
    SH2Context cpu{};
    sh2_call(cpu, entry);
    assert(calls_a == 1 && calls_b == 0);
    // Recarga por st32: as duas imagens compartilham exatamente a mesma entrada.
    for (size_t i = 0; i < after.size(); i += 4) {
        uint32_t word = uint32_t(after[i]) << 24 | uint32_t(after[i+1]) << 16 |
                        uint32_t(after[i+2]) << 8 | after[i+3];
        st32(header + uint32_t(i), word);
    }
    sh2_call(cpu, entry);
    assert(calls_a == 1 && calls_b == 1);
    assert(!g_sh2_residency_dirty);
    // Não há CRC por callback: dados do jogo fora do cabeçalho não sujam a carga.
    st32(0x060A2000, 42);
    assert(!g_sh2_residency_dirty);
    sh2_call(cpu, entry);
    assert(calls_b == 2 && !g_sh2_residency_dirty);
    // Carga em bloco e alias cache-through precisam passar pelo mesmo mecanismo.
    assert(sh2_mem_write(0x260DC000, before.data(), before.size()));
    sh2_call(cpu, 0x260DC060);
    assert(calls_a == 2 && calls_b == 2);
    // Mirrors de 1MiB também invalidam o cabeçalho.
    sh2_io_write(0x061DC000, ld32(header), 4);
    assert(g_sh2_residency_dirty);
    sh2_call(cpu, entry);
    assert(calls_a == 3 && !g_sh2_residency_dirty);
    // Escrita que começa antes da faixa e a intercepta, sem confundir outros dados.
    sh2_residency_write(header - 2, 4);
    assert(g_sh2_residency_dirty);
    sh2_call(cpu, entry);
    // Se a nova imagem estiver incompleta, não executar o callback anterior.
    st8(header, ld8(header)); st8(base + 1, ld8(base + 1) ^ 0xFF);
    int old_calls = calls_a + calls_b;
    bool refused = false;
    try { sh2_call(cpu, entry); } catch (Unrecognized&) { refused = true; }
    assert(refused && calls_a + calls_b == old_calls && g_sh2_residency_dirty);
    if (argc == 3) {
        auto read = [](const char* path) {
            std::ifstream input(path, std::ios::binary);
            if (!input) throw std::runtime_error("fixture missing");
            return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
        };
        auto original_ma = read(argv[1]), captured_stage03 = read(argv[2]);
        assert(original_ma.size() == 48492 && captured_stage03.size() == 147456);
        const SH2FuncEntry real_a[] = {{0x060DC318u, a}}, real_b[] = {{0x060DC318u, b}};
        ma.size = 0x9158; ma.crc = 0x0CB2F7AD; ma.funcs = real_a;
        mb.size = 103904; mb.crc = 0xCFFFB4E4; mb.funcs = real_b;
        sh2_watch_residency(header, 64, base, 0x06100000);
        assert(sh2_mem_write(header, original_ma.data(), original_ma.size()));
        sh2_activate(&ma);
        sh2_call(cpu, 0x060DC318);
        int before_reload = calls_b;
        assert(sh2_mem_write(header, captured_stage03.data(), captured_stage03.size()));
        sh2_call(cpu, 0x060DC318);
        assert(calls_b == before_reload + 1 && sh2_identify(base) == &mb);
        std::puts("PASS: captured Maria failure dispatches STAGE_03 instead of stale STAGE_MA");
    }
    if (argc == 1) {
        // A busca precisa aceitar outra base dentro do domínio e recusar ambiguidade.
        const SH2FuncEntry lower_entries[] = {{header + 0x20, b}, {entry, b}};
        mb.base = header + 0x20; mb.size = 160; mb.funcs = lower_entries; mb.nfuncs = 2;
        mb.crc = sh2_crc32(after.data() + 0x20, mb.size);
        ma.crc ^= 0x1234; // Somente a imagem mb é candidata exata.
        assert(sh2_mem_write(header, after.data(), after.size()));
        int count = calls_b;
        sh2_call(cpu, header + 0x20);
        assert(calls_b == count + 1 && !g_sh2_residency_dirty);
        ma.base = mb.base; ma.size = mb.size; ma.crc = mb.crc;
        ma.funcs = lower_entries; ma.nfuncs = 2;
        sh2_residency_write(header, 4);
        bool ambiguous_refused = false;
        try { sh2_call(cpu, entry); } catch (Unrecognized&) { ambiguous_refused = true; }
        assert(ambiguous_refused && calls_b == count + 1 && g_sh2_residency_dirty);
        // Módulo com CRC correto fora do domínio vigiado não pode ser selecionado.
        ma.base = header - 0x20; ma.size = 224;
        ma.crc = sh2_crc32(g_wram_h + (ma.base & 0xFFFFF), ma.size);
        mb.crc ^= 1;
        bool outside_refused = false;
        try { sh2_call(cpu, entry); } catch (Unrecognized&) { outside_refused = true; }
        assert(outside_refused && calls_b == count + 1);
        std::puts("PASS: alternate base, ambiguous identity and out-of-domain refusal");
    }
    if (argc == 4) {
        auto read = [](const char* path) {
            std::ifstream input(path, std::ios::binary);
            if (!input) throw std::runtime_error("fixture missing");
            return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
        };
        auto original = read(argv[1]), captured = read(argv[2]);
        uint32_t end = uint32_t(std::stoul(argv[3]));
        assert(original.size() >= 0x14B74 && captured.size() == 147456 && end == 0x14190);
        const SH2FuncEntry real_a[] = {{base, a}}, real_b[] = {{base, b}};
        ma.size = 0x14B34; ma.crc = sh2_crc32(original.data() + 64, ma.size); ma.funcs = real_a;
        mb.size = end - 64; mb.crc = sh2_crc32(original.data() + 64, mb.size); mb.funcs = real_b;
        assert(ma.crc == 0x4CA6B6D8 && mb.crc == 0x3024B3DA);
        sh2_watch_residency(header, 64, base, 0x06100000);
        assert(sh2_mem_write(header, original.data(), ma.size + 64));
        sh2_activate(&ma);
        // Sem a identidade corrigida, os bytes reais reproduzem a recusa antiga.
        mb.crc ^= 1;
        assert(sh2_mem_write(header, captured.data(), captured.size()));
        int before_calls = calls_a + calls_b;
        bool old_refused = false;
        try { sh2_call(cpu, base); } catch (Unrecognized&) { old_refused = true; }
        assert(old_refused && calls_a + calls_b == before_calls);
        mb.crc ^= 1;
        sh2_call(cpu, base);
        assert(sh2_identify(base) == &mb && calls_a + calls_b == before_calls + 1);
        // Excluir os campos gráficos não pode tornar uma instrução alterada válida.
        captured[64] ^= 1;
        assert(sh2_mem_write(header, captured.data(), captured.size()));
        assert(sh2_identify(base) == nullptr);
        bool corrupted_refused = false;
        try { sh2_call(cpu, base); } catch (Unrecognized&) { corrupted_refused = true; }
        assert(corrupted_refused && calls_a + calls_b == before_calls + 1);
        std::puts("PASS: captured STAGE_15 rejects old range, dispatches fixed identity and refuses corrupted instruction");
    }
    if (argc == 5) {
        std::ifstream source(argv[1], std::ios::binary), crash(argv[2], std::ios::binary);
        std::vector<uint8_t> original{std::istreambuf_iterator<char>(source), {}};
        std::vector<uint8_t> captured{std::istreambuf_iterator<char>(crash), {}};
        assert(captured.size() == 147456 && original.size() >= 0x2A2C);
        const SH2FuncEntry ending_entries[] = {{0x060DC020u, b}, {0x060DC6ACu, b}};
        ma.base = 0x060DC040; ma.size = 128; ma.crc = 0xBAD00BAD; ma.funcs = entries_a; ma.nfuncs = 1;
        mb.base = 0x060DC020; mb.size = 0x2A0C; mb.crc = 0x400B6243; mb.funcs = ending_entries; mb.nfuncs = 2;
        assert(sh2_crc32(original.data() + 0x20, mb.size) == mb.crc);
        sh2_watch_residency(header, 64, base, 0x06100000);
        assert(sh2_mem_write(header, captured.data(), captured.size()));
        sh2_activate(&ma);
        int before_calls = calls_b;
        sh2_call(cpu, 0x060DC020);
        assert(calls_b == before_calls + 1 && !g_sh2_residency_dirty);
        sh2_call(cpu, 0x060DC6AC);
        assert(calls_b == before_calls + 2 && sh2_lookup(0x060DC6AC) == b);
        // Mesmo se ENDING já foi ativado, uma carga suja exige CRC completo.
        sh2_residency_write(header, 4);
        sh2_call(cpu, 0x260DC6AC);
        assert(calls_b == before_calls + 3 && !g_sh2_residency_dirty);
        captured[0x20] ^= 1;
        assert(sh2_mem_write(header, captured.data(), captured.size()));
        bool invalid_refused = false;
        try { sh2_call(cpu, 0x060DC6AC); } catch (Unrecognized&) { invalid_refused = true; }
        assert(invalid_refused && calls_b == before_calls + 3 && g_sh2_residency_dirty);
        std::puts("PASS: captured ENDING at offset20 dispatches and corrupted code stays refused");
    }
    std::puts("PASS: residency reload, alias, mirrors, block writes and unknown-image refusal");
}
