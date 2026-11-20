#pragma once

#include <SDL3/SDL.h>
#include "sotn_gpu_shader_dxil.h"
#include "sotn_gpu_shader_spirv.h"

// Chamadas no thread do renderer; fechar antes de destruir o SDL_Renderer.
class SotnGpuShader {
public:
    SotnGpuShader() = default;
    SotnGpuShader(const SotnGpuShader&) = delete;
    SotnGpuShader& operator=(const SotnGpuShader&) = delete;
    ~SotnGpuShader() { close(); }

    bool open(SDL_Renderer* renderer) {
        close();
        if (!renderer) return fail("Renderer ausente");
        SDL_GPUDevice* device = SDL_GetGPURendererDevice(renderer);
        if (!device) return fail("Shader requer renderer SDL_GPU (D3D12 ou Vulkan)");
        const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
        if ((formats & (SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV)) == 0) {
            return fail("Dispositivo sem suporte DXIL ou SPIR-V");
        }
        renderer_ = renderer;
        device_ = device;
        SDL_GPUShaderCreateInfo shaderInfo{};
        if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
            shaderInfo.code = kSotnGpuDxil;
            shaderInfo.code_size = sizeof(kSotnGpuDxil);
            shaderInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
        } else {
            shaderInfo.code = kSotnGpuSpirv;
            shaderInfo.code_size = sizeof(kSotnGpuSpirv);
            shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        }
        shaderInfo.entrypoint = "main";
        shaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        shaderInfo.num_samplers = 1;
        shaderInfo.num_uniform_buffers = 1;
        shader_ = SDL_CreateGPUShader(device_, &shaderInfo);
        if (!shader_) return failSDL("Criar shader GPU");
        SDL_GPURenderStateCreateInfo stateInfo{};
        stateInfo.fragment_shader = shader_;
        state_ = SDL_CreateGPURenderState(renderer_, &stateInfo);
        if (!state_) return failSDL("Criar estado GPU");
        const char* driver = SDL_GetGPUDeviceDriver(device_);
        SDL_snprintf(status_, sizeof(status_), "Shaders GPU %s disponiveis (%s)",
                     driver ? driver : "desconhecido", shaderInfo.format == SDL_GPU_SHADERFORMAT_DXIL ? "DXIL" : "SPIR-V");
        return true;
    }

    bool enabled() const { return state_ != nullptr; }

    void disable(const char* reason) {
        fail(reason ? reason : "Shader GPU desativado");
    }

    // effect: 0 original, 1 CRT, 2 LCD, 3 Scale2x, 4 Scale3x.
    bool begin(int effect, int width, int height) {
        end();
        if (effect == 0) return true;
        if (!state_ || effect < 1 || effect > 4 || width <= 0 || height <= 0) return false;
        const float uniforms[4] = {static_cast<float>(width), static_cast<float>(height),
                                   static_cast<float>(effect), 0.0f};
        if (!SDL_SetGPURenderStateFragmentUniforms(state_, 0, uniforms, sizeof(uniforms)) ||
            !SDL_SetGPURenderState(renderer_, state_)) {
            SDL_snprintf(status_, sizeof(status_), "Aplicar shader: %.120s", SDL_GetError());
            return false;
        }
        return true;
    }

    void end() {
        if (state_ && renderer_) SDL_SetGPURenderState(renderer_, nullptr);
    }

    void close() {
        end();
        if (state_) SDL_DestroyGPURenderState(state_);
        if (shader_ && device_) SDL_ReleaseGPUShader(device_, shader_);
        state_ = nullptr;
        shader_ = nullptr;
        device_ = nullptr;
        renderer_ = nullptr;
    }

    const char* status() const { return status_; }

private:
    SDL_Renderer* renderer_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUShader* shader_ = nullptr;
    SDL_GPURenderState* state_ = nullptr;
    char status_[192] = "Shader GPU nao inicializado";

    bool fail(const char* message) {
        close();
        SDL_strlcpy(status_, message, sizeof(status_));
        return false;
    }

    bool failSDL(const char* operation) {
        char message[192];
        SDL_snprintf(message, sizeof(message), "%s: %.120s", operation, SDL_GetError());
        return fail(message);
    }
};
