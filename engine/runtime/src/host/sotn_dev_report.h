#pragma once
#include "sotn_bug_report.h"

namespace SotnDevReport {
inline void append(std::string& target, const char* utf8, std::size_t limit) {
    const std::string value(utf8);
    if (target.size() + value.size() <= limit) target += value;
}
inline void backspace(std::string& target) {
    if (target.empty()) return;
    auto n = target.size() - 1;
    while (n > 0 && (static_cast<unsigned char>(target[n]) & 0xc0u) == 0x80u) --n;
    target.resize(n);
}
inline bool save(const std::filesystem::path& folder, const std::string& title, const std::string& description) {
    if (folder.empty() || !std::filesystem::exists(folder / "report.json")) return false;
    return SotnBugReport::text_file(folder / "relato.json", "{\n  \"titulo\": " + SotnBugReport::quoted(title) +
        ",\n  \"descricao\": " + SotnBugReport::quoted(description) + "\n}\n");
}
}
