#pragma once
#include <SDL3/SDL.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace SotnBugReport {
namespace fs = std::filesystem;
using Fields = std::vector<std::pair<std::string, std::string>>;
struct Result {
    bool created = false, rendered = false, original = false;
    fs::path folder;
    std::string error;
};

inline std::string quoted(const std::string& text) {
    const char* digits = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 32) { out += "\\u00"; out += digits[c >> 4]; out += digits[c & 15]; }
        else out += char(c);
    }
    return out + '"';
}

inline bool text_file(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(text.data(), std::streamsize(text.size()));
    stream.close();
    return bool(stream);
}

inline bool copy_tail(const fs::path& source, const fs::path& target, std::streamoff limit) {
    std::ifstream input(source, std::ios::binary | std::ios::ate);
    if (!input) return false;
    auto size = input.tellg();
    if (size < 0) return false;
    input.seekg(size > limit ? size - limit : std::streampos(0));
    std::ofstream output(target, std::ios::binary);
    if (!output) return false;
    if (input.peek() != std::char_traits<char>::eof()) output << input.rdbuf();
    output.close();
    return bool(output);
}

// Chamar após desenhar o jogo e antes da interface/presentação. O readback é
// feito só sob demanda; nenhuma leituraGPU adicional durante a partida normal.
inline Result capture(SDL_Renderer* renderer, const fs::path& session, uint64_t vblank,
                      const uint32_t* pixels, int width, int height,
                      Fields fields, const std::string& settings) {
    Result result;
    std::error_code ec;
    const auto reports = session / "bugs";
    fs::create_directories(reports, ec);
    if (ec) { result.error = ec.message(); return result; }
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    for (int attempt = 0; attempt < 100; ++attempt) {
        result.folder = reports / ("bug-" + std::to_string(timestamp) + "-vb" + std::to_string(vblank) + "-" + std::to_string(attempt));
        if (fs::create_directory(result.folder, ec)) break;
        if (ec) { result.error = ec.message(); return result; }
        if (attempt == 99) { result.error = "Nao foi possivel reservar pasta unica"; return result; }
    }
    if (renderer) {
        SDL_Surface* rendered = SDL_RenderReadPixels(renderer, nullptr);
        if (rendered) {
            result.rendered = SDL_SavePNG(rendered, (result.folder / "rendered.png").string().c_str());
            if (!result.rendered) fields.emplace_back("rendered_capture_error", SDL_GetError());
            fields.emplace_back("rendered_width", std::to_string(rendered->w));
            fields.emplace_back("rendered_height", std::to_string(rendered->h));
            SDL_DestroySurface(rendered);
        } else fields.emplace_back("rendered_capture_error", SDL_GetError());
    } else fields.emplace_back("rendered_capture_error", "Renderizador indisponivel");
    if (pixels && width > 0 && height > 0 && int64_t(width) * height <= 32 * 1024 * 1024) {
        SDL_Surface* original = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_XRGB8888);
        if (original) {
            for (int y = 0; y < height; ++y)
                std::memcpy(static_cast<unsigned char*>(original->pixels) + size_t(y) * size_t(original->pitch),
                            pixels + size_t(y) * size_t(width), size_t(width) * sizeof(uint32_t));
            result.original = SDL_SavePNG(original, (result.folder / "original.png").string().c_str());
            SDL_DestroySurface(original);
        }
        if (!result.original) fields.emplace_back("original_capture_error", SDL_GetError());
    } else fields.emplace_back("original_capture_error", "Quadro original indisponivel");
    if (renderer) {
        fields.emplace_back("renderer", SDL_GetRendererName(renderer));
        if (auto* device = SDL_GetGPURendererDevice(renderer)) {
            fields.emplace_back("gpu_backend", SDL_GetGPUDeviceDriver(device));
            auto properties = SDL_GetGPUDeviceProperties(device);
            for (const auto& item : Fields{{"gpu_name", SDL_PROP_GPU_DEVICE_NAME_STRING},
                    {"gpu_driver", SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING},
                    {"gpu_driver_version", SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING}})
                fields.emplace_back(item.first, SDL_GetStringProperty(properties, item.second.c_str(), "indisponivel"));
        }
    }
    fields.emplace_back("vblank", std::to_string(vblank));
    fields.emplace_back("timestamp_unix_ms", std::to_string(timestamp));
    fields.emplace_back("sdl_version", std::to_string(SDL_GetVersion()));
    fields.emplace_back("original_width", std::to_string(width));
    fields.emplace_back("original_height", std::to_string(height));
    fields.emplace_back("settings_saved", text_file(result.folder / "settings.ini", settings) ? "true" : "false");
    // Somente caudas dos logs e a rota da sessão. Nunca copiar backup/save.
    for (const auto* name : {"run.log", "stdout.log"})
        fields.emplace_back(std::string(name) + "_copied", copy_tail(session / name, result.folder / name, 65536) ? "true" : "false");
    fields.emplace_back("input_copied", copy_tail(session / "input.txt", result.folder / "input.txt", 16 * 1024 * 1024) ? "true" : "false");
    fields.emplace_back("input_snapshot_limit_bytes", "16777216");
    fields.emplace_back("session_metadata_copied", copy_tail(session / "session.json", result.folder / "session.json", 65536) ? "true" : "false");
    std::string json = "{\n  \"schema_version\": 1,\n  \"rendered_saved\": ";
    json += result.rendered ? "true" : "false";
    json += ",\n  \"original_saved\": ";
    json += result.original ? "true" : "false";
    for (const auto& field : fields) json += ",\n  " + quoted(field.first) + ": " + quoted(field.second);
    json += "\n}\n";
    result.created = text_file(result.folder / "report.json", json);
    if (!result.created) result.error = "Falha ao gravar report.json";
    text_file(result.folder / "LEIA-ME.txt",
        "Relatorio local de renderizacao SotN Saturn.\n"
        "original.png: quadro Saturn antes dos filtros do host.\n"
        "rendered.png: quadro com escala/filtros/shaders, antes do menu Esc.\n"
        "report.json registra falhas de captura; imagem ausente nao significa jogo sem bug.\n"
        "settings.ini e caudas dos logs ajudam o diagnostico; input.txt pode ser parcial.\n"
        "Acrescente em descricao.txt: o que aconteceu e o que deveria aparecer.\n"
        "Nao inclui save pessoal, ROM ou BIOS. Nada enviado automaticamente.\n");
    return result;
}
}
