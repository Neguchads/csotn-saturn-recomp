#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace SotnVideo {

struct Resolution { const char* name; int width; int height; };
inline constexpr std::array<Resolution, 8> kResolutions{{
    {"640x480", 640, 480}, {"960x720", 960, 720},
    {"1280x720", 1280, 720}, {"1280x960", 1280, 960},
    {"1920x1080", 1920, 1080},
    {"1920x1440", 1920, 1440}, {"2560x1440", 2560, 1440}, {"3840x2160", 3840, 2160},
}};
inline constexpr std::array<unsigned, 6> kWindowScales{{1, 2, 3, 4, 5, 6}};
inline constexpr std::array<unsigned, 6> kSpeeds{{1, 2, 4, 6, 8, 10}};
inline constexpr std::array<int, 5> kHdFactors{{1, 2, 4, 6, 8}};
inline constexpr std::array<const char*, 9> kAspectNames{{"TV 4:3", "Pixels", "Preencher", "16:9", "16:10", "21:9", "3:2", "5:4", "1:1"}};
inline constexpr std::array<std::array<int, 2>, 9> kAspectRatios{{
    {{4,3}}, {{0,0}}, {{0,0}}, {{16,9}}, {{16,10}}, {{21,9}}, {{3,2}}, {{5,4}}, {{1,1}}
}};

enum class Effect { Original, Scanlines, Lcd };
inline constexpr std::array<const char*, 3> kEffectNames{{"Original", "Scanlines", "Mascara LCD"}};

// FSR tem prioridade sobre HD: um HD inativo nesse quadro não falhou.
constexpr bool hd_failed(bool fsrRequested, int requestedFactor, int activeFactor, bool originalTexture) {
    return !fsrRequested && requestedFactor > 1 && (activeFactor != requestedFactor || originalTexture);
}

// A preferência explícita do jogador prevalece; o launcher só orienta o modo automático.
constexpr int gpu_backend(int configured, std::string_view preferred) {
    if (configured != 0) return configured;
    if (preferred == "vulkan") return 3;
    if (preferred == "direct3d12") return 1;
    return 0;
}

struct Rect { int x; int y; int w; int h; };

// Ajuste comum corrige a imagem Saturn para 4:3; inteiro preserva seus pixels nativos.
inline Rect viewport(int boundsW, int boundsH, int frameW, int frameH, bool integerScale, int aspect = 0) {
    if (boundsW <= 0 || boundsH <= 0 || frameW <= 0 || frameH <= 0) return {};
    if (aspect == 2) return {0, 0, boundsW, boundsH};
    int w = 0;
    int h = 0;
    // Proporções explícitas têm prioridade; inteiro vale para TV/pixels.
    const bool nativeInteger = integerScale && (aspect == 0 || aspect == 1);
    if (nativeInteger) {
        const int scale = std::min(boundsW / frameW, boundsH / frameH);
        if (scale > 0) {
            w = frameW * scale;
            h = frameH * scale;
        }
    }
    if (w == 0 || h == 0) {
        const int selected = aspect >= 0 && aspect < int(kAspectRatios.size()) ? aspect : 0;
        const std::int64_t aspectW = nativeInteger || selected == 1 ? frameW : kAspectRatios[selected][0];
        const std::int64_t aspectH = nativeInteger || selected == 1 ? frameH : kAspectRatios[selected][1];
        if (std::int64_t(boundsW) * aspectH <= std::int64_t(boundsH) * aspectW) {
            w = boundsW;
            h = static_cast<int>(std::int64_t(w) * aspectH / aspectW);
        } else {
            h = boundsH;
            w = static_cast<int>(std::int64_t(h) * aspectW / aspectH);
        }
        // Uma janela muito estreita ainda recebe ao menos um pixel em cada eixo.
        w = std::max(w, 1);
        h = std::max(h, 1);
    }
    return {(boundsW - w) / 2, (boundsH - h) / 2, w, h};
}

// Filtros por pixel no framebuffer XRGB8888, sem shader GPU ou estado entre quadros.
inline std::uint32_t effect(std::uint32_t pixel, int x, int y, Effect mode) {
    unsigned red = (pixel >> 16) & 255;
    unsigned green = (pixel >> 8) & 255;
    unsigned blue = pixel & 255;
    if (mode == Effect::Scanlines && y % 2 != 0) {
        red = red * 3 / 4;
        green = green * 3 / 4;
        blue = blue * 3 / 4;
    } else if (mode == Effect::Lcd) {
        const int column = (x % 3 + 3) % 3;
        if (column != 0) red = red * 220 / 256;
        if (column != 1) green = green * 220 / 256;
        if (column != 2) blue = blue * 220 / 256;
    }
    return (pixel & 0xFF000000u) | (red << 16) | (green << 8) | blue;
}

} // namespace SotnVideo
