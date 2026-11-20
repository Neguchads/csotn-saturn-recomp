#pragma once
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <utility>
#include <vector>

// Formato Saturn restrito à Work RAM; nenhuma escrita passa pelo barramento MMIO.
namespace SotnMods {
struct Script {
    std::string name;
    bool enabled = false;

private:
    struct Code {
        unsigned type;
        uint32_t address;
        uint16_t value;
        bool applied = false;
    };
    std::vector<Code> codes;
    friend class Engine;
};

class Engine {
    static constexpr size_t MaxScripts = 64;
    static constexpr size_t MaxCodes = 256;
    static constexpr size_t MaxLine = 512;
    static constexpr size_t MaxBytes = 1024 * 1024;

    static std::string trim(const std::string& text) {
        const auto first = text.find_first_not_of(" \t\r");
        if (first == std::string::npos) return {};
        return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
    }

    static bool hex(const std::string& text, size_t digits, uint32_t& value) {
        if (text.size() != digits) return false;
        value = 0;
        for (const unsigned char c : text) {
            unsigned digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else return false;
            value = (value << 4) | digit;
        }
        return true;
    }

    static bool work_ram(uint32_t address, bool word) {
        const bool in_ram = (address >= 0x06000000 && address <= 0x060FFFFF) ||
                            (address >= 0x00200000 && address <= 0x002FFFFF);
        return in_ram && (!word || (address & 1) == 0);
    }

    bool fail(size_t line, const char* reason) {
        error = "Linha " + std::to_string(line) + ": " + reason;
        return false;
    }

public:
    std::vector<Script> scripts;
    std::string error;

    bool load(std::istream& input) {
        std::vector<Script> parsed;
        std::string line;
        size_t line_number = 0;
        size_t bytes = 0;
        while (true) {
            line.clear();
            bool consumed = false;
            char c;
            while (input.get(c)) {
                consumed = true;
                if (++bytes > MaxBytes) return fail(line_number + 1, "arquivo excede 1 MiB");
                if (c == '\n') break;
                if (line.size() == MaxLine) return fail(line_number + 1, "linha excede 512 bytes");
                line.push_back(c);
            }
            if (input.bad() || (input.fail() && !input.eof()))
                return fail(line_number + 1, "falha de leitura");
            if (!consumed) break;
            ++line_number;
            const auto comment = line.find_first_of("#;");
            line = trim(line.substr(0, comment));
            if (line.empty()) continue;
            if (line.front() == '[') {
                if (!parsed.empty() && (parsed.back().codes.empty() || parsed.back().codes.back().type == 0xD))
                    return fail(line_number, "secao vazia ou condicao D sem escrita seguinte");
                if (line.back() != ']') return fail(line_number, "secao invalida");
                const auto name = trim(line.substr(1, line.size() - 2));
                if (name.empty() || name.size() > 32) return fail(line_number, "nome deve ter de 1 a 32 caracteres ASCII");
                for (const unsigned char ch : name)
                    if (ch < 32 || ch > 126 || ch == '[' || ch == ']')
                        return fail(line_number, "nome deve usar ASCII imprimivel");
                if (parsed.size() == MaxScripts) return fail(line_number, "limite de 64 scripts");
                for (const auto& script : parsed)
                    if (script.name == name) return fail(line_number, "nome de script duplicado");
                Script script;
                script.name = name;
                parsed.push_back(std::move(script));
                continue;
            }
            if (parsed.empty()) return fail(line_number, "codigo exige secao [Nome]");
            const auto space = line.find_first_of(" \t");
            if (space == std::string::npos) return fail(line_number, "use XXXXXXXX YYYY");
            uint32_t encoded = 0, value = 0;
            if (!hex(line.substr(0, space), 8, encoded) || !hex(trim(line.substr(space)), 4, value))
                return fail(line_number, "use XXXXXXXX YYYY em hexadecimal");
            const unsigned type = encoded >> 28;
            if (type != 0 && type != 1 && type != 3 && type != 0xD)
                return fail(line_number, "tipo Saturn nao suportado; master/F/B e codigos PS1 sao recusados");
            auto& codes = parsed.back().codes;
            if (!codes.empty() && codes.back().type == 0xD && type == 0xD)
                return fail(line_number, "condicoes D encadeadas nao sao suportadas");
            const uint32_t address = encoded & 0x0FFFFFFF;
            if (!work_ram(address, type != 3)) return fail(line_number, "endereco exige Work RAM Saturn e alinhamento de 16 bits");
            if (type == 3 && value > 0xFF) return fail(line_number, "escrita de 8 bits exige valor ate 00FF");
            if (codes.size() == MaxCodes) return fail(line_number, "limite de 256 codigos por script");
            codes.push_back({type, address, static_cast<uint16_t>(value), false});
        }
        if (parsed.empty()) return fail(line_number + 1, "nenhum script");
        if (parsed.back().codes.empty() || parsed.back().codes.back().type == 0xD)
            return fail(line_number + 1, "secao vazia ou condicao D sem escrita seguinte");
        scripts = std::move(parsed);
        error.clear();
        return true;
    }

    void disable_all() {
        for (auto& script : scripts) {
            script.enabled = false;
            for (auto& code : script.codes) code.applied = false;
        }
    }

    void set_enabled(size_t index, bool enabled) {
        if (index >= scripts.size()) return;
        auto& script = scripts[index];
        if (!enabled || !script.enabled)
            for (auto& code : script.codes) code.applied = false;
        script.enabled = enabled;
    }

    template<class Read8, class Write8>
    void apply(Read8 read8, Write8 write8) {
        for (auto& script : scripts) {
            if (!script.enabled) {
                for (auto& code : script.codes) code.applied = false;
                continue;
            }
            for (size_t i = 0; i < script.codes.size(); ++i) {
                auto& code = script.codes[i];
                if (code.type == 0xD) {
                    const uint16_t actual = static_cast<uint16_t>(
                        (static_cast<uint16_t>(read8(code.address)) << 8) |
                        static_cast<uint16_t>(read8(code.address + 1)));
                    if (actual != code.value) ++i;
                    continue;
                }
                if (code.type == 0 && code.applied) continue;
                if (code.type == 3) {
                    write8(code.address, static_cast<uint8_t>(code.value));
                } else {
                    write8(code.address, static_cast<uint8_t>(code.value >> 8));
                    write8(code.address + 1, static_cast<uint8_t>(code.value));
                }
                if (code.type == 0) code.applied = true;
            }
        }
    }
};
} // namespace SotnMods
