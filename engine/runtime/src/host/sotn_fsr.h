#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include "sotn_fsr_dxil.h"
#include "sotn_fsr_spirv.h"

// Integra os passes EASU/RCAS AMD. Recursos locais ao renderer, sem histórico temporal.
class SotnFsr {
public:
    SotnFsr() = default;
    SotnFsr(const SotnFsr&) = delete;
    SotnFsr& operator=(const SotnFsr&) = delete;
    ~SotnFsr() { close(); }
    bool open(SDL_Renderer* renderer) {
        close();
        if (!renderer || !(device_ = SDL_GetGPURendererDevice(renderer))) return fail("FSR1 requer GPU Vulkan ou D3D12");
        renderer_ = renderer;
        const bool dxil = (SDL_GetGPUShaderFormats(device_) & SDL_GPU_SHADERFORMAT_DXIL) != 0;
        for (int pass = 0; pass < 2; ++pass) {
            SDL_GPUShaderCreateInfo info{};
            if (dxil) {
                info.code = pass == 0 ? kSotnFsrEasuDxil : kSotnFsrRcasDxil;
                info.code_size = pass == 0 ? sizeof(kSotnFsrEasuDxil) : sizeof(kSotnFsrRcasDxil);
                info.format = SDL_GPU_SHADERFORMAT_DXIL;
            } else {
                info.code = pass == 0 ? kSotnFsrEasuSpirv : kSotnFsrRcasSpirv;
                info.code_size = pass == 0 ? sizeof(kSotnFsrEasuSpirv) : sizeof(kSotnFsrRcasSpirv);
                info.format = SDL_GPU_SHADERFORMAT_SPIRV;
            }
            info.entrypoint = "main";
            info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
            info.num_samplers = 1;
            info.num_uniform_buffers = 1;
            shaders_[pass] = SDL_CreateGPUShader(device_, &info);
            if (!shaders_[pass]) return failSDL("Criar shader FSR1");
            SDL_GPURenderStateCreateInfo state{};
            state.fragment_shader = shaders_[pass];
            states_[pass] = SDL_CreateGPURenderState(renderer_, &state);
            if (!states_[pass]) return failSDL("Criar estado FSR1");
        }
        SDL_strlcpy(status_, "FSR1 EASU + RCAS disponivel", sizeof status_);
        return true;
    }
    bool enabled() const { return states_[0] && states_[1]; }
    const char* status() const { return status_; }
    SDL_Texture* render(SDL_Texture* source, int width, int height, float sharpness) {
        if (!source || !enabled()) return source;
        const auto max = SDL_GetNumberProperty(SDL_GetRendererProperties(renderer_), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
        if (width < source->w || height < source->h || width <= 0 || height <= 0 || width > max || height > max ||
            std::int64_t(width) * height > 33554432 || !std::isfinite(sharpness)) return source;
        if (width != width_ || height != height_) {
            releaseTargets();
            for (auto& target : targets_) {
                target = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
                if (!target || !SDL_SetTextureBlendMode(target, SDL_BLENDMODE_NONE)) { failSDL("Alocar FSR1"); return source; }
            }
            width_ = width; height_ = height;
        }
        struct Uniforms { std::array<std::uint32_t,16> con{}; float dimensions[4]{}; } uniforms;
        static_assert(sizeof(Uniforms) == 80);
        const float iw = float(source->w), ih = float(source->h), ow = float(width), oh = float(height);
        // Mesmas constantes FsrEasuCon oficiais; bit_cast preserva os bits float em uint.
        const std::array<float,16> easu{{iw/ow,ih/oh,0.5f*iw/ow-0.5f,0.5f*ih/oh-0.5f,
            1/iw,1/ih,1/iw,-1/ih,-1/iw,2/ih,1/iw,2/ih,0,4/ih,0,0}};
        for (size_t i = 0; i < easu.size(); ++i) uniforms.con[i] = std::bit_cast<std::uint32_t>(easu[i]);
        uniforms.dimensions[0]=iw; uniforms.dimensions[1]=ih; uniforms.dimensions[2]=ow; uniforms.dimensions[3]=oh;
        SDL_Texture* previous = SDL_GetRenderTarget(renderer_);
        SDL_ScaleMode original;
        if (!SDL_GetTextureScaleMode(source, &original)) return source;
        SDL_Texture* input = source;
        bool ok = true;
        for (int pass = 0; pass < 2; ++pass) {
            if (pass == 1) {
                uniforms.con = {};
                uniforms.con[0] = std::bit_cast<std::uint32_t>(std::exp2(-std::clamp(sharpness,0.0f,2.0f)));
                uniforms.dimensions[0]=ow; uniforms.dimensions[1]=oh;
            }
            ok = SDL_SetTextureScaleMode(input, SDL_SCALEMODE_LINEAR) && SDL_SetRenderTarget(renderer_, targets_[pass]) &&
                SDL_SetGPURenderStateFragmentUniforms(states_[pass],0,&uniforms,sizeof uniforms) &&
                SDL_SetGPURenderState(renderer_,states_[pass]) && SDL_RenderTexture(renderer_,input,nullptr,nullptr);
            SDL_SetGPURenderState(renderer_,nullptr);
            if (!ok) break;
            input = targets_[pass];
        }
        const bool targetRestored = SDL_SetRenderTarget(renderer_,previous);
        const bool modeRestored = SDL_SetTextureScaleMode(source,original);
        const bool restored = targetRestored && modeRestored;
        if (!ok || !restored) { failSDL("Desenhar FSR1"); return source; }
        SDL_snprintf(status_, sizeof status_, "FSR1 EASU + RCAS %dx%d",width,height);
        return input;
    }
    void close() {
        if (renderer_ && enabled()) SDL_SetGPURenderState(renderer_,nullptr);
        releaseTargets();
        for (auto& state : states_) { if (state) SDL_DestroyGPURenderState(state); state=nullptr; }
        for (auto& shader : shaders_) { if (shader && device_) SDL_ReleaseGPUShader(device_,shader); shader=nullptr; }
        renderer_=nullptr; device_=nullptr;
    }
private:
    SDL_Renderer* renderer_=nullptr;
    SDL_GPUDevice* device_=nullptr;
    std::array<SDL_GPUShader*,2> shaders_{};
    std::array<SDL_GPURenderState*,2> states_{};
    std::array<SDL_Texture*,2> targets_{};
    int width_=0,height_=0;
    char status_[192]="FSR1 desligado";
    void releaseTargets() { for(auto& t:targets_) { if(t) SDL_DestroyTexture(t); t=nullptr; } width_=height_=0; }
    bool fail(const char* reason) { close(); SDL_strlcpy(status_,reason,sizeof status_); return false; }
    bool failSDL(const char* prefix) { char message[192]; SDL_snprintf(message,sizeof message,"%s: %.130s",prefix,SDL_GetError()); return fail(message); }
};
