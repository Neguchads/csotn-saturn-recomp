#include "sotn_mods.h"
#include "sotn_presets.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Ram {
    std::map<uint32_t, uint8_t> bytes;
    unsigned reads = 0;
    unsigned writes = 0;

    static bool valid(uint32_t address) {
        return (address >= 0x06000000 && address <= 0x060FFFFF) ||
               (address >= 0x00200000 && address <= 0x002FFFFF);
    }
    uint8_t read(uint32_t address) {
        require(valid(address), "Read reached ROM/MMIO/outside RAM");
        ++reads;
        return bytes[address];
    }
    void write(uint32_t address, uint8_t value) {
        require(valid(address), "Write reached ROM/MMIO/outside RAM");
        ++writes;
        bytes[address] = value;
    }
    void apply(SotnMods::Engine& engine) {
        engine.apply([this](uint32_t address) { return read(address); },
                     [this](uint32_t address, uint8_t value) { write(address, value); });
    }
};

static bool load(SotnMods::Engine& engine, const std::string& text) {
    std::istringstream stream(text);
    return engine.load(stream);
}

static void writes_and_activation() {
    SotnMods::Engine engine;
    require(load(engine,
        "# arquivo local\r\n[One shot]\r\n06000000 1234 ; comentario\r\n"
        "[Continuous]\n16000002 ABcd\n30200001 00Ef\n"
        "[Edges]\n060ffffe FEED\n102ffffe BEEF\n360fffff 007F\n"), "Valid codes must load");
    require(engine.scripts.size() == 3 && engine.error.empty(), "Expected three scripts");
    Ram ram;
    ram.apply(engine);
    require(ram.reads == 0 && ram.writes == 0, "Loaded scripts start disabled");
    engine.set_enabled(0, true);
    ram.apply(engine);
    require(ram.bytes[0x06000000] == 0x12 && ram.bytes[0x06000001] == 0x34, "16-bit write must be big-endian");
    ram.bytes[0x06000000] = 0;
    ram.apply(engine);
    require(ram.bytes[0x06000000] == 0 && ram.writes == 2, "One-shot must not repeat");
    engine.set_enabled(0, false);
    engine.set_enabled(0, true);
    ram.apply(engine);
    require(ram.bytes[0x06000000] == 0x12 && ram.writes == 4, "Immediate off/on resets one-shot");
    engine.set_enabled(0, true);
    ram.apply(engine);
    require(ram.writes == 4, "Setting enabled true twice must not reset one-shot");
    engine.set_enabled(1, true);
    ram.apply(engine);
    require(ram.bytes[0x06000002] == 0xAB && ram.bytes[0x06000003] == 0xCD && ram.bytes[0x00200001] == 0xEF,
            "Continuous writes must support both RAM regions and odd byte address");
    ram.bytes[0x06000002] = 0;
    ram.bytes[0x00200001] = 0;
    ram.apply(engine);
    require(ram.bytes[0x06000002] == 0xAB && ram.bytes[0x00200001] == 0xEF, "Continuous writes repeat");
    engine.set_enabled(2, true);
    ram.apply(engine);
    require(ram.bytes[0x060FFFFE] == 0xFE && ram.bytes[0x002FFFFF] == 0xEF && ram.bytes[0x060FFFFF] == 0x7F,
            "Upper RAM boundaries must be safe");
    engine.disable_all();
    const auto writes = ram.writes;
    ram.apply(engine);
    require(ram.writes == writes, "Disable all stops writes");
    engine.set_enabled(0, true);
    ram.apply(engine);
    require(ram.writes == writes + 2, "Disable all resets one-shots");
    engine.set_enabled(100, true);
}

static void conditional_writes() {
    SotnMods::Engine engine;
    require(load(engine,
        "[Condition]\nD6000010 1234\n06000020 ABCD\n16000022 1357\n"
        "D0200010 5678\n30200020 0042\n"), "D conditions must load");
    Ram ram;
    engine.set_enabled(0, true);
    ram.apply(engine);
    require(!ram.bytes.count(0x06000020) && !ram.bytes.count(0x00200020), "False condition skips exactly its next write");
    require(ram.bytes[0x06000022] == 0x13, "Write after conditional pair remains unconditional");
    ram.bytes[0x06000010] = 0x12;
    ram.bytes[0x06000011] = 0x34;
    ram.bytes[0x00200010] = 0x56;
    ram.bytes[0x00200011] = 0x78;
    ram.apply(engine);
    require(ram.bytes[0x06000020] == 0xAB && ram.bytes[0x06000021] == 0xCD && ram.bytes[0x00200020] == 0x42,
            "Conditions read big-endian words and pending one-shot waits for true condition");
    ram.bytes[0x06000020] = 0;
    ram.bytes[0x00200020] = 0;
    ram.apply(engine);
    require(ram.bytes[0x06000020] == 0 && ram.bytes[0x00200020] == 0x42, "Conditional one-shot fires once, byte write repeats");
    engine.set_enabled(0, false);
    engine.set_enabled(0, true);
    ram.apply(engine);
    require(ram.bytes[0x06000020] == 0xAB, "Conditional one-shot resets after disabling");
}

static void rejected_files_are_atomic() {
    const std::string invalid[] = {
        "", "# empty\n", "06000000 0001\n", "[Empty]\n", "[Unclosed\n06000000 0001\n",
        "[A]\nF6000000 0001\n", "[A]\nB6000000 0001\n", "[A]\n86000000 0001\n",
        "[A]\n80010000 0001\n", "[A]\n50000000 0001\n", "[A]\n26000000 0001\n",
        "[A]\n10000000 0001\n", "[A]\n16010001 0001\n", "[A]\nD0200001 0001\n",
        "[A]\n102FFFFF 0001\n", "[A]\n160FFFFF 0001\n", "[A]\n10600000 0001\n",
        "[A]\n125D0000 0001\n", "[A]\n12600000 0001\n", "[A]\n36000000 0100\n",
        "[A]\n36000000 1000\n", "[A]\nD6000000 0001\n",
        "[A]\nD6000000 0001\nD6000002 0002\n06000004 0003\n",
        "[A]\nD6000000 0001\n[B]\n06000004 0003\n",
        "[A]\n06000000 0001\n[A]\n06000002 0002\n",
        "[A]\n0600000 0001\n", "[A]\n06000000 001\n", "[A]\n06000000 00001\n",
        "[A]\n0x600000 0001\n", "[A]\n06000000 0001 junk\n", "[A]\nGG000000 0001\n",
        "[A]\n06000000 0001\n16010001 0002\n",
        "[A]\n06000000 0001\nF6000002 0002\n",
        "[" + std::string(33, 'X') + "]\n06000000 0001\n",
        "[N\xC3\xA3oASCII]\n06000000 0001\n",
        "[A[B]]\n06000000 0001\n",
        "[A]\n" + std::string(513, '#') + "\n",
        "[A]\n06000000 0001\n" + std::string(1024 * 1024, '\n'),
    };
    SotnMods::Engine engine;
    require(load(engine, "[Original]\n06000000 1234\n"), "Baseline must load");
    engine.set_enabled(0, true);
    Ram ram;
    ram.apply(engine);
    ram.bytes[0x06000000] = 0;
    for (const auto& text : invalid) {
        const auto writes = ram.writes;
        require(!load(engine, text), "Invalid file must be rejected entirely");
        require(engine.error.starts_with("Linha "), "Error must name its line");
        require(engine.scripts.size() == 1 && engine.scripts[0].name == "Original" && engine.scripts[0].enabled,
                "Failed load preserves script and activation");
        ram.apply(engine);
        require(ram.writes == writes && ram.bytes[0x06000000] == 0, "Failed load preserves one-shot state and performs no new write");
    }
    require(load(engine, "[Replacement]\n06000002 FFFF\n"), "Reload must succeed");
    require(engine.error.empty() && engine.scripts.size() == 1 && !engine.scripts[0].enabled,
            "Successful reload clears error and disables every script");
}

static void limits_and_io_failure() {
    SotnMods::Engine engine;
    std::string scripts;
    for (unsigned i = 0; i < 64; ++i) scripts += "[Script" + std::to_string(i) + "]\n06000000 0001\n";
    require(load(engine, scripts), "64 scripts must load");
    require(!load(engine, scripts + "[Overflow]\n06000000 0001\n"), "65 scripts must fail");
    require(engine.scripts.size() == 64, "Script overflow is atomic");
    std::string codes = "[Codes]\n";
    for (unsigned i = 0; i < 256; ++i) codes += "06000000 0001\n";
    require(load(engine, codes), "256 codes must load");
    require(!load(engine, codes + "06000002 0001\n"), "257 codes must fail");
    require(engine.scripts.size() == 1 && engine.scripts[0].name == "Codes", "Code overflow is atomic");
    require(load(engine, "[" + std::string(32, 'X') + "]\n06000000 0001"), "32 ASCII name and EOF without newline are valid");
    std::istringstream broken("[Broken]\n06000002 0002\n");
    broken.setstate(std::ios::badbit);
    require(!engine.load(broken) && engine.scripts[0].name == std::string(32, 'X'), "Read failure preserves old scripts");
}

static void poke32(Ram& ram, uint32_t address, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) ram.bytes[address + i] = static_cast<uint8_t>(value >> (24 - 8 * i));
}

static void poke16(Ram& ram, uint32_t address, uint16_t value) {
    ram.bytes[address] = static_cast<uint8_t>(value >> 8);
    ram.bytes[address + 1] = static_cast<uint8_t>(value);
}

static uint32_t peek32(const Ram& ram, uint32_t address) {
    uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value = (value << 8) | ram.bytes.at(address + i);
    return value;
}

static Ram gameplay_ram() {
    Ram ram;
    poke16(ram, 0x0605CEA2, 0x11);
    poke16(ram, 0x0605CD70, 5);
    poke32(ram, 0x0605C6D0, 0);
    poke16(ram, 0x0605D750, 12);
    poke32(ram, 0x0605C940, 21);
    poke32(ram, 0x0605C944, 1000);
    poke32(ram, 0x0605C950, 1);
    poke32(ram, 0x0605C954, 1234);
    poke32(ram, 0x0605C948, 2);
    poke32(ram, 0x0605C94C, 777);
    poke32(ram, 0x0605C990, 50);
    poke16(ram, 0x0605C522, 0x4567); // timer13 pertence às cenas.
    poke16(ram, 0x0605C524, 0x1234);
    poke32(ram, 0x0605D7E0, 0);
    poke32(ram, 0x0605C538, 0);
    return ram;
}

static bool presets_apply(SotnPresets::State& state, Ram& ram) {
    return SotnPresets::apply(state,
        [&ram](uint32_t address) { return ram.read(address); },
        [&ram](uint32_t address, uint8_t value) { ram.write(address, value); });
}

static void presets_only_write_valid_gameplay() {
    auto ram = gameplay_ram();
    SotnPresets::State state;
    // Este caso verifica especificamente os quatro presets originais;
    // os novos limites e atributos possuem casos independentes abaixo.
    for (size_t i = 0; i < 4; ++i) state.enabled[i] = true;
    require(presets_apply(state, ram), "Presets must accept gameplay fixture");
    require(peek32(ram, 0x0605C940) == 1000 && ram.bytes[0x0605C942] == 3 && ram.bytes[0x0605C943] == 0xE8,
            "God HP uses a big-endian 32-bit maximum");
    require(ram.bytes[0x0605C524] == 0 && ram.bytes[0x0605C525] == 30, "God must refresh timer14 to 30");
    require(ram.bytes[0x0605C522] == 0x45 && ram.bytes[0x0605C523] == 0x67, "God must preserve timer13");
    require(peek32(ram, 0x0605C950) == 1234 && peek32(ram, 0x0605C948) == 777, "Mana and hearts use their own maxima");
    require(peek32(ram, 0x0605C990) == 999999, "Alucard gold must be 999999");
    state.disable_all();
    const auto writes = ram.writes;
    require(presets_apply(state, ram) && ram.writes == writes, "All disabled presets perform zero writes");
    for (uint32_t character = 1; character <= 2; ++character) {
        ram = gameplay_ram();
        poke32(ram, 0x0605C6D0, character);
        state.enabled[SotnPresets::Gold] = true;
        require(presets_apply(state, ram) && ram.writes == 0 && peek32(ram, 0x0605C990) == 50,
                "Gold must not modify Richter/Maria");
    }
    struct GuardCase { uint32_t address; uint32_t value; bool word; };
    const GuardCase guards[] = {
        {0x0605CEA2, 0, true}, {0x0605CD70, 0, true}, // boot/título fora de gameplay.
        {0x0605C6D0, 3, false}, {0x0605D750, 100, true},
        {0x0605C940, 0, false}, {0x0605C940, 1001, false},
        {0x0605C944, 0, false}, {0x0605C944, 10000, false},
        {0x0605D7E0, 1, false}, {0x0605C538, 0x40000, false}, // cena/morte.
    };
    for (const auto& guard : guards) {
        ram = gameplay_ram();
        if (guard.word) poke16(ram, guard.address, static_cast<uint16_t>(guard.value));
        else poke32(ram, guard.address, guard.value);
        state.enabled.fill(true);
        state.heal = true;
        require(!presets_apply(state, ram) && ram.writes == 0, "Invalid gameplay guard must prevent every write");
        require(state.heal, "Heal waits for gameplay instead of being lost during boot/cutscene");
    }
    ram = gameplay_ram();
    state.disable_all();
    state.heal = true;
    require(presets_apply(state, ram) && !state.heal, "Heal is consumed once after gameplay");
    require(peek32(ram, 0x0605C940) == 1000 && peek32(ram, 0x0605C950) == 1234 && peek32(ram, 0x0605C948) == 777,
            "Heal restores HP, mana and hearts");
    require(ram.bytes[0x0605C524] == 0x12 && ram.bytes[0x0605C525] == 0x34 && peek32(ram, 0x0605C990) == 50,
            "Heal alone preserves invincibility timer and gold");
    const auto healed_writes = ram.writes;
    presets_apply(state, ram);
    require(ram.writes == healed_writes, "Heal must not repeat");
    ram = gameplay_ram();
    state.enabled[SotnPresets::Mana] = state.enabled[SotnPresets::Hearts] = true;
    poke32(ram, 0x0605C954, 10000);
    poke32(ram, 0x0605C94C, 10000);
    require(presets_apply(state, ram) && ram.writes == 0, "Implausible maxima cannot become resource writes");
}

static void expanded_presets_have_bounded_character_specific_writes() {
    using namespace SotnPresets;
    require(Count == 15 && Gold == 3 && Health == 4 && AllRelics == 12,
            "Original preset IDs must remain stable and relic presets must be appended");
    State state;
    auto ram = gameplay_ram();
    require(presets_apply(state, ram) && ram.writes == 0, "New presets must default off");
    for (size_t index = 0; index < Count; ++index)
        require(labels[index][0] && descriptions[index][0], "Every preset must have a label and help");
    state.enabled[Health] = true;
    require(presets_apply(state, ram) && peek32(ram, 0x0605C940) == 1000,
            "Health-only preset must refill HP");
    require(ram.bytes[0x0605C524] == 0x12 && ram.bytes[0x0605C525] == 0x34,
            "Health-only preset must not add invincibility");
    struct ResourceCase { Id id; uint32_t current, maximum, target; };
    const ResourceCase resources[] = {
        {MaxHealth, 0x0605C940, 0x0605C944, 999},
        {MaxMana, 0x0605C950, 0x0605C954, 999},
        {MaxHearts, 0x0605C948, 0x0605C94C, 99},
    };
    for (const auto& resource : resources) {
        state.disable_all(); state.enabled[resource.id] = true;
        ram = gameplay_ram();
        poke32(ram, resource.maximum, 50); poke32(ram, resource.current, 21);
        require(presets_apply(state, ram) && peek32(ram, resource.current) == resource.target &&
                peek32(ram, resource.maximum) == resource.target, "Resource preset raises current and maximum");
        poke32(ram, resource.maximum, 1234);
        require(presets_apply(state, ram) && peek32(ram, resource.maximum) == 1234,
                "Resource preset must preserve superior maxima");
        for (uint32_t character : {1u, 2u}) {
            ram = gameplay_ram(); poke32(ram, 0x0605C6D0, character);
            require(presets_apply(state, ram) && ram.writes == 0,
                    "New maximum presets must not write Richter or Maria");
        }
        if (resource.id != MaxHealth) {
            ram = gameplay_ram(); poke32(ram, resource.maximum, 10000);
            require(presets_apply(state, ram) && ram.writes == 0, "Invalid resource maximum must not be patched");
        }
    }
    for (unsigned stat = 0; stat < 4; ++stat) {
        state.disable_all(); state.enabled[Strength + stat] = true;
        ram = gameplay_ram();
        const uint32_t address = 0x0605C958 + 4 * stat;
        poke32(ram, address, 12);
        require(presets_apply(state, ram) && peek32(ram, address) == 99 && ram.writes == 4,
                "Selected base stat alone must be raised using big-endian 32-bit writes");
        poke32(ram, address, 150); ram.writes = 0;
        require(presets_apply(state, ram) && peek32(ram, address) == 150 && ram.writes == 0,
                "Superior base stat must be preserved");
        for (uint32_t character : {1u, 2u}) {
            ram = gameplay_ram(); poke32(ram, 0x0605C6D0, character);
            require(presets_apply(state, ram) && ram.writes == 0,
                    "Alucard base stat presets must not modify Richter or Maria");
        }
    }
}

static void relic_grants_are_one_shot_and_preserve_familiar_selection() {
    using namespace SotnPresets;
    for (Id id : {AllRelics, Transformations, Mobility}) {
        State state;
        auto ram = gameplay_ram();
        constexpr uint32_t base = 0x0605C6F0;
        ram.bytes[base - 1] = 0xAA; ram.bytes[base + 32] = 0xBB;
        ram.bytes[base + 31] = 0x80; // Entrada oculta: não disponibilizar.
        ram.bytes[base + 22] = 3;    // Espada já selecionada no menu original.
        state.set_enabled(id, true);
        require(presets_apply(state, ram), "Relic grant accepts valid Alucard gameplay");
        for (unsigned i = 0; i < 31; ++i) {
            const bool selected = id == AllRelics || (id == Transformations && i < 10) ||
                                  (id == Mobility && (i == 12 || i == 13 || i == 30));
            const uint8_t original = i == 22 ? 3 : 0;
            const uint8_t expected = selected ? uint8_t(original | (i >= 18 && i <= 24 ? 1 : 3)) : original;
            require(ram.read(base + i) == expected, "Grant must touch exactly the selected relic flags");
        }
        require(ram.bytes[base - 1] == 0xAA && ram.bytes[base + 32] == 0xBB && ram.bytes[base + 31] == 0x80,
                "Grant preserves adjacent bytes and hidden slot31");
        const auto writes = ram.writes;
        ram.bytes[base] = 0;
        presets_apply(state, ram);
        require(ram.writes == writes && ram.bytes[base] == 0, "Grant must not reapply every frame");
        state.set_enabled(id, true);
        presets_apply(state, ram);
        require(ram.writes == writes, "Setting an enabled grant on again is not a new activation");
        state.set_enabled(id, false); state.set_enabled(id, true);
        presets_apply(state, ram);
        require(ram.bytes[base] == (id == Mobility ? 0 : 3), "Off/on rearms the selected grant");
        state.disable_all();
        ram = gameplay_ram();
        presets_apply(state, ram);
        require(ram.writes == 0, "Disable all clears relic grants");
        state.set_enabled(id, true);
        presets_apply(state, ram);
        require(ram.writes > 0, "Grant can be enabled after disable-all");
        for (uint32_t character : {1u, 2u}) {
            state.disable_all(); state.set_enabled(id, true);
            ram = gameplay_ram(); poke32(ram, 0x0605C6D0, character);
            presets_apply(state, ram);
            require(ram.writes == 0, "Relic grants must not change Richter/Maria");
            poke32(ram, 0x0605C6D0, 0); presets_apply(state, ram);
            require(ram.writes > 0, "Relic grant waits rather than being consumed by another character");
        }
        state.disable_all(); state.set_enabled(id, true);
        ram = gameplay_ram(); poke16(ram, 0x0605CD70, 0);
        require(!presets_apply(state, ram) && ram.writes == 0, "Grant waits through title/loading");
        poke16(ram, 0x0605CD70, 5); presets_apply(state, ram);
        require(ram.writes > 0, "Pending grant applies after gameplay begins");
    }
}

int main(int argc, char** argv) {
    try {
        writes_and_activation();
        conditional_writes();
        rejected_files_are_atomic();
        limits_and_io_failure();
        presets_only_write_valid_gameplay();
        expanded_presets_have_bounded_character_specific_writes();
        relic_grants_are_one_shot_and_preserve_familiar_selection();
        if (argc == 2) {
            std::ifstream input(argv[1]);
            require(bool(input), "Bundled scripts must exist");
            SotnMods::Engine scripts;
            require(scripts.load(input), "Bundled Saturn GameShark scripts must parse");
            require(scripts.scripts.size() == 4, "Bundled file must expose four scripts");
            Ram ram;
            auto read = [&](uint32_t a) { return ram.read(a); };
            auto write = [&](uint32_t a, uint8_t v) { ram.write(a, v); };
            scripts.apply(read, write);
            require(ram.writes == 0, "Bundled scripts must start disabled");
            for (size_t i = 0; i < scripts.scripts.size(); ++i) scripts.set_enabled(i, true);
            scripts.apply(read, write);
            require(SotnPresets::read32(read, 0x0605C940) == 999, "Bundled HP must be BE32 999");
            require(SotnPresets::read32(read, 0x0605C954) == 999, "Bundled MP max must be BE32 999");
            require(SotnPresets::read32(read, 0x0605C948) == 99, "Bundled hearts must be BE32 99");
            require(SotnPresets::read32(read, 0x0605C990) == 999999, "Bundled gold condition must execute");
        }
        std::puts("PASS: Saturn mod parser and executor");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
