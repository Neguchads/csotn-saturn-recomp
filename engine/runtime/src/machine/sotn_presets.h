#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Endereços e tipos da edição Saturn JP; não reutilizar os offsets de PS1.
namespace SotnPresets {
// Preservar os quatro IDs existentes; novos presets vêm depois.
enum Id { God, Mana, Hearts, Gold, Health, MaxHealth, MaxMana, MaxHearts,
          Strength, Constitution, Intelligence, Luck, AllRelics, Transformations, Mobility, Count };
inline constexpr std::array<const char*, Count> labels = {
    "Godmode (vida + invencivel)", "Mana infinita", "Coracoes infinitos", "Ouro 999999 (Alucard)",
    "Vida infinita (permite dano)", "HP maximo 999 (Alucard)", "MP maximo 999 (Alucard)",
    "Coracoes maximos 99 (Alucard)", "STR base 99 (Alucard)", "CON base 99 (Alucard)",
    "INT base 99 (Alucard)", "LCK base 99 (Alucard)",
    "Todas as reliquias (Alucard)", "Transformacoes completas (Alucard)", "Mobilidade completa (Alucard)"
};
inline constexpr std::array<const char*, Count> descriptions = {
    "Recarrega HP e timer de invencibilidade durante gameplay; nao avanca cenas.",
    "Recarrega MP ate o maximo atual; nao aumenta o limite.",
    "Recarrega coracoes ate o maximo atual; nao aumenta o limite.",
    "Fixa ouro em 999999, somente Alucard. Pode persistir se salvar.",
    "Recarrega HP sem alterar invencibilidade: golpes e knockback continuam.",
    "Eleva HP atual/maximo a pelo menos 999. Somente Alucard; persiste se salvar.",
    "Eleva MP atual/maximo a pelo menos 999. Somente Alucard; persiste se salvar.",
    "Eleva coracoes atuais/maximos a pelo menos 99. Alucard; persiste se salvar.",
    "Eleva STR base a pelo menos 99. Totais seguem o recalculo do jogo; persiste se salvar.",
    "Eleva CON base a pelo menos 99. Totais seguem o recalculo do jogo; persiste se salvar.",
    "Eleva INT base a pelo menos 99. Totais seguem o recalculo do jogo; persiste se salvar.",
    "Eleva LCK base a pelo menos 99. Totais seguem o recalculo do jogo; persiste se salvar.",
    "Libera as 31 reliquias disponiveis, incluindo botas Saturn. Familiares ficam para selecionar no menu original. Aplica uma vez por ativacao; persiste se salvar.",
    "Libera morcego, lobo, nevoa e seus poderes. Uma vez por ativacao; desligue/ligue para repetir. Persiste se salvar.",
    "Libera salto duplo, super salto e botas de corrida Saturn. Uma vez por ativacao; desligue/ligue para repetir. Persiste se salvar."
};
struct State {
    std::array<bool, Count> enabled{};
    std::array<bool, Count> applied{};
    bool heal = false;
    void set_enabled(size_t index, bool value) {
        if (index >= enabled.size()) return;
        if (enabled[index] != value) applied[index] = false;
        enabled[index] = value;
    }
    void disable_all() { enabled.fill(false); applied.fill(false); heal = false; }
};
template<class Read8> uint32_t read32(Read8 read, uint32_t a) {
    return (uint32_t(read(a)) << 24) | (uint32_t(read(a + 1)) << 16) |
           (uint32_t(read(a + 2)) << 8) | uint32_t(read(a + 3));
}
template<class Read8> uint16_t read16(Read8 read, uint32_t a) {
    return uint16_t((uint16_t(read(a)) << 8) | read(a + 1));
}
template<class Write8> void write32(Write8 write, uint32_t a, uint32_t v) {
    for (int i = 0; i < 4; ++i) write(a + uint32_t(i), uint8_t(v >> (24 - i * 8)));
}
template<class Read8> bool gameplay(Read8 read) {
    const uint32_t maximum = read32(read, 0x0605C944), hp = read32(read, 0x0605C940);
    return read16(read, 0x0605CEA2) == 0x11 && read16(read, 0x0605CD70) == 5 &&
           read32(read, 0x0605C6D0) <= 2 && read16(read, 0x0605D750) < 100 &&
           maximum > 0 && maximum <= 9999 && hp > 0 && hp <= maximum &&
           read32(read, 0x0605D7E0) == 0 && (read32(read, 0x0605C538) & 0x40000) == 0;
}
template<class Read8, class Write8> bool apply(State& state, Read8 read, Write8 write) {
    if (!gameplay(read)) return false;
    const bool alucard = read32(read, 0x0605C6D0) == 0;
    // PlayerStatus Saturn: hp +0x250, mpMax +0x264, statsBase +0x268.
    // Aumentos sao pisos, nunca reduzem um maximo/base ja superior ao preset.
    if (alucard) {
        // PlayerStatus Saturn começa com relics[32]. O slot31 é ocultado pelo
        // menu original e não deve ser forçado. Familiares18..24 usam FOUND;
        // preservar ACTIVE mantém a seleção existente sem convocar sete juntos.
        for (const Id grant : {AllRelics, Transformations, Mobility}) {
            if (!state.enabled[grant] || state.applied[grant]) continue;
            for (unsigned i = 0; i < 31; ++i) {
                const bool selected = grant == AllRelics || (grant == Transformations && i < 10) ||
                                      (grant == Mobility && (i == 12 || i == 13 || i == 30));
                if (!selected) continue;
                const uint32_t address = 0x0605C6F0 + i;
                const uint8_t previous = read(address);
                const uint8_t value = uint8_t(previous | (i >= 18 && i <= 24 ? 1 : 3));
                if (value != previous) write(address, value);
            }
            state.applied[grant] = true;
        }
        struct Resource { Id id; uint32_t current, maximum, floor; };
        for (const auto& resource : std::array<Resource, 3>{{
                 {MaxHealth, 0x0605C940, 0x0605C944, 999},
                 {MaxMana, 0x0605C950, 0x0605C954, 999},
                 {MaxHearts, 0x0605C948, 0x0605C94C, 99}}}) {
            const uint32_t maximum = read32(read, resource.maximum);
            if (state.enabled[resource.id] && maximum > 0 && maximum <= 9999) {
                const uint32_t target = maximum < resource.floor ? resource.floor : maximum;
                write32(write, resource.maximum, target);
                write32(write, resource.current, target);
            }
        }
        for (unsigned i = 0; i < 4; ++i) {
            const auto id = static_cast<Id>(Strength + i);
            const uint32_t address = 0x0605C958 + i * 4;
            const uint32_t base = read32(read, address);
            if (state.enabled[id] && base < 99) write32(write, address, 99);
        }
    }
    if (state.enabled[God] || state.enabled[Health] || state.heal) {
        write32(write, 0x0605C940, read32(read, 0x0605C944));
        if (state.enabled[God]) {
            // Timer14 impede hit detection; timer13 é usado por cenas e permanece intacto.
            write(0x0605C524, 0); write(0x0605C525, 30);
        }
    }
    if (state.enabled[Mana] || state.heal) {
        uint32_t maximum = read32(read, 0x0605C954);
        if (maximum <= 9999) write32(write, 0x0605C950, maximum);
    }
    if (state.enabled[Hearts] || state.heal) {
        uint32_t maximum = read32(read, 0x0605C94C);
        if (maximum <= 9999) write32(write, 0x0605C948, maximum);
    }
    // Ouro é parte da campanha Alucard; não alterar o status dos outros personagens.
    if (state.enabled[Gold] && alucard) write32(write, 0x0605C990, 999999);
    state.heal = false;
    return true;
}
} // namespace SotnPresets
