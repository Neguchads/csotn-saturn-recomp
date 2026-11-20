#pragma once

#include "sotn_gpu_shader.h"
#include <array>
#include <cstdint>

// Passes so para a imagem do jogo. O host desenha UI apos render() retornar.
class SotnHdScaler {
public:
    SotnHdScaler() = default;
    SotnHdScaler(const SotnHdScaler&) = delete;
    SotnHdScaler& operator=(const SotnHdScaler&) = delete;
    ~SotnHdScaler() { close(); }

    bool open(SDL_Renderer* renderer) {
        close();
        if (!shader_.open(renderer)) {
            SDL_strlcpy(status_, shader_.status(), sizeof(status_));
            return false;
        }
        renderer_ = renderer;
        SDL_strlcpy(status_, "HD GPU disponivel", sizeof(status_));
        return true;
    }

    bool resize(int width, int height, int factor) {
        if (width <= 0 || height <= 0 ||
            (factor != 1 && factor != 2 && factor != 4 && factor != 6 && factor != 8)) {
            return fail("Dimensoes ou fator HD invalidos");
        }
        if (factor == 1) {
            releaseTargets();
            width_ = width;
            height_ = height;
            SDL_strlcpy(status_, "HD desligado", sizeof(status_));
            return true;
        }
        if (!renderer_ || !shader_.enabled()) return fail("HD requer renderer GPU ativo");
        const Sint64 maximum = SDL_GetNumberProperty(SDL_GetRendererProperties(renderer_),
            SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
        // Limite tambem contem alocacoes acidentais de centenas de MB.
        const std::int64_t outW = std::int64_t(width) * factor;
        const std::int64_t outH = std::int64_t(height) * factor;
        if (maximum <= 0 || outW > maximum || outH > maximum || outW > 33554432 / outH) {
            return fail("Resolucao HD excede limite da GPU ou 32 megapixels");
        }
        if (factor_ == factor && width_ == width && height_ == height) return true;
        releaseTargets();
        width_ = width;
        height_ = height;
        const int count = factor == 2 ? 1 : factor == 8 ? 3 : 2;
        int w = width;
        int h = height;
        for (int pass = 0; pass < count; ++pass) {
            const int step = factor == 6 && pass == 1 ? 3 : 2;
            w *= step;
            h *= step;
            steps_[pass] = step;
            targets_[pass] = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                               SDL_TEXTUREACCESS_TARGET, w, h);
            if (!targets_[pass] || !SDL_SetTextureScaleMode(targets_[pass], SDL_SCALEMODE_NEAREST) ||
                !SDL_SetTextureBlendMode(targets_[pass], SDL_BLENDMODE_NONE)) {
                return failSDL("Criar alvo HD");
            }
        }
        passes_ = count;
        factor_ = factor;
        SDL_snprintf(status_, sizeof(status_), "HD GPU %dx (%dx%d)", factor_, w, h);
        return true;
    }

    SDL_Texture* render(SDL_Texture* source) {
        if (!source || factor_ == 1 || !renderer_) return source;
        if (source->w != width_ || source->h != height_) {
            fail("Fonte HD mudou; redimensione antes de desenhar");
            return source;
        }
        SDL_Texture* previous = SDL_GetRenderTarget(renderer_);
        SDL_Texture* input = source;
        bool success = true;
        for (int pass = 0; pass < passes_; ++pass) {
            SDL_ScaleMode originalMode;
            if (!SDL_GetTextureScaleMode(input, &originalMode)) { success = false; break; }
            const bool nearest = SDL_SetTextureScaleMode(input, SDL_SCALEMODE_NEAREST);
            const bool target = nearest && SDL_SetRenderTarget(renderer_, targets_[pass]);
            const bool begun = target && shader_.begin(steps_[pass] == 2 ? 3 : 4, input->w, input->h);
            const bool drawn = begun && SDL_RenderTexture(renderer_, input, nullptr, nullptr);
            shader_.end();
            const bool restored = SDL_SetTextureScaleMode(input, originalMode);
            if (!drawn || !restored) { success = false; break; }
            input = targets_[pass];
        }
        shader_.end();
        const bool targetRestored = SDL_SetRenderTarget(renderer_, previous);
        if (!success || !targetRestored) {
            failSDL("Desenhar HD");
            return source;
        }
        return input;
    }

    int factor() const { return factor_; }
    const char* status() const { return status_; }

    void close() {
        releaseTargets();
        shader_.close();
        renderer_ = nullptr;
        width_ = height_ = 0;
    }

private:
    SDL_Renderer* renderer_ = nullptr;
    SotnGpuShader shader_;
    std::array<SDL_Texture*, 3> targets_{};
    std::array<int, 3> steps_{};
    int width_ = 0;
    int height_ = 0;
    int factor_ = 1;
    int passes_ = 0;
    char status_[192] = "HD desligado";

    void releaseTargets() {
        for (SDL_Texture*& texture : targets_) {
            if (texture) SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        factor_ = 1;
        passes_ = 0;
    }

    bool fail(const char* reason) {
        releaseTargets();
        SDL_strlcpy(status_, reason, sizeof(status_));
        return false;
    }

    bool failSDL(const char* operation) {
        char message[192];
        SDL_snprintf(message, sizeof(message), "%s: %.120s", operation, SDL_GetError());
        return fail(message);
    }
};
