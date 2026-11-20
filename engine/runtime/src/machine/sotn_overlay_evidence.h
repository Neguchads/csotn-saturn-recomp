#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace SotnOverlayEvidence {
inline constexpr uint32_t kBase=0x060DC000, kSize=0x24000;
// Apenas a janela dos overlays; não grava RAM baixa, backup nem arquivo de save.
enum class FailureReason { Fatal, ProgramStartIdentityMismatch };

inline const char* failure_filename(FailureReason reason) {
    switch (reason) {
    case FailureReason::Fatal: return "overlay-failure-fatal.bin";
    case FailureReason::ProgramStartIdentityMismatch: return "overlay-failure-identity-mismatch.bin";
    }
    return "overlay-failure-unknown.bin";
}

inline const char* failure_reason_name(FailureReason reason) {
    switch (reason) {
    case FailureReason::Fatal: return "fatal";
    case FailureReason::ProgramStartIdentityMismatch: return "program-start identity mismatch";
    }
    return "unknown";
}

inline std::filesystem::path failure_path(const std::filesystem::path& output,FailureReason reason) {
    return output/failure_filename(reason);
}

template<class Read>
bool capture(const std::filesystem::path& path,Read read) {
    std::vector<uint8_t> bytes(kSize);
    if (!read(kBase,bytes.data(),bytes.size())) return false;
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));
    file.close();
    return bool(file);
}
}
