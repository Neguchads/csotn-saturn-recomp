#pragma once
#include <SDL3/SDL.h>
#include "video.h"
#include "vdp2_gpu_snapshot.h"
#include "sotn_vdp2_dxil.h"
#include "sotn_vdp2_spirv.h"
#include "video_perf.h"
#include <cstring>

// Um quadro em voo e fence antes de reutilizar buffers. Sem janela ou thread extra.
class SotnVdp2Gpu {
public:
    SotnVdp2Gpu() = default;
    SotnVdp2Gpu(const SotnVdp2Gpu&) = delete;
    SotnVdp2Gpu& operator=(const SotnVdp2Gpu&) = delete;
    ~SotnVdp2Gpu() { close(); }
    bool open(const char* backend = nullptr) {
        close();
        device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV, false, backend);
        if (!device_) return false;
        SDL_GPUComputePipelineCreateInfo info{};
        if (SDL_GetGPUShaderFormats(device_) & SDL_GPU_SHADERFORMAT_DXIL) {
            info.code = kVdp2Dxil; info.code_size = sizeof kVdp2Dxil; info.format = SDL_GPU_SHADERFORMAT_DXIL;
        } else {
            info.code = kVdp2Spirv; info.code_size = sizeof kVdp2Spirv; info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        }
        info.entrypoint = "main";
        info.num_readonly_storage_buffers = 1; info.num_readwrite_storage_buffers = 1;
        info.num_uniform_buffers = 1; info.threadcount_x = 8; info.threadcount_y = 8; info.threadcount_z = 1;
        pipeline_ = SDL_CreateGPUComputePipeline(device_, &info);
        if (!pipeline_) return fail();
        SDL_GPUBufferCreateInfo buffer{};
        buffer.size = kInputSize; buffer.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
        input_ = SDL_CreateGPUBuffer(device_, &buffer);
        buffer.size = kOutputSize; buffer.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        output_ = SDL_CreateGPUBuffer(device_, &buffer);
        SDL_GPUTransferBufferCreateInfo transfer{};
        transfer.size = kInputSize; transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        upload_ = SDL_CreateGPUTransferBuffer(device_, &transfer);
        transfer.size = kOutputSize; transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
        download_ = SDL_CreateGPUTransferBuffer(device_, &transfer);
        if (!input_ || !output_ || !upload_ || !download_) return fail();
        return true;
    }
    const char* backend() const { return device_ ? SDL_GetGPUDeviceDriver(device_) : "disabled"; }
    bool compose(const Vdp2GpuSnapshot& snapshot, Frame& result) {
        VideoPerf::Scope profile(VideoPerf::Stage::GpuCompose);
        const auto width = snapshot.params[0], height = snapshot.params[1];
        if (!pipeline_ || !width || !height || width > 704 || height > 512 ||
            snapshot.bytes.size() != 0x81200 + size_t(width) * height * 4) return false;
        const Uint32 inputSize = static_cast<Uint32>(snapshot.bytes.size()), outputSize = width * height * 4;
        auto* mapped = SDL_MapGPUTransferBuffer(device_, upload_, false);
        if (!mapped) return fail();
        std::memcpy(mapped, snapshot.bytes.data(), inputSize);
        SDL_UnmapGPUTransferBuffer(device_, upload_);
        SDL_GPUCommandBuffer* command = SDL_AcquireGPUCommandBuffer(device_);
        if (!command) return fail();
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(command);
        SDL_GPUTransferBufferLocation source{upload_, 0};
        SDL_GPUBufferRegion target{input_, 0, inputSize};
        SDL_UploadToGPUBuffer(copy, &source, &target, false);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUStorageBufferReadWriteBinding binding{}; binding.buffer = output_;
        SDL_GPUComputePass* compute = SDL_BeginGPUComputePass(command, nullptr, 0, &binding, 1);
        SDL_BindGPUComputePipeline(compute, pipeline_);
        SDL_BindGPUComputeStorageBuffers(compute, 0, &input_, 1);
        SDL_PushGPUComputeUniformData(command, 0, snapshot.params.data(), sizeof snapshot.params);
        SDL_DispatchGPUCompute(compute, (width + 7) / 8, (height + 7) / 8, 1);
        SDL_EndGPUComputePass(compute);
        copy = SDL_BeginGPUCopyPass(command);
        SDL_GPUBufferRegion rendered{output_, 0, outputSize};
        SDL_GPUTransferBufferLocation destination{download_, 0};
        SDL_DownloadFromGPUBuffer(copy, &rendered, &destination);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
        if (!fence) return fail();
        bool completed;
        { VideoPerf::Scope wait(VideoPerf::Stage::GpuWait); completed = SDL_WaitForGPUFences(device_, true, &fence, 1); }
        SDL_ReleaseGPUFence(device_, fence);
        if (!completed) return fail();
        mapped = SDL_MapGPUTransferBuffer(device_, download_, false);
        if (!mapped) return fail();
        result.w = int(width); result.h = int(height); result.px.resize(size_t(width) * height);
        std::memcpy(result.px.data(), mapped, outputSize);
        SDL_UnmapGPUTransferBuffer(device_, download_);
        return true;
    }
    void close() {
        if (!device_) return;
        SDL_WaitForGPUIdle(device_);
        if (pipeline_) SDL_ReleaseGPUComputePipeline(device_, pipeline_);
        if (input_) SDL_ReleaseGPUBuffer(device_, input_);
        if (output_) SDL_ReleaseGPUBuffer(device_, output_);
        if (upload_) SDL_ReleaseGPUTransferBuffer(device_, upload_);
        if (download_) SDL_ReleaseGPUTransferBuffer(device_, download_);
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr; pipeline_ = nullptr; input_ = output_ = nullptr; upload_ = download_ = nullptr;
    }
private:
    bool fail() {
        const std::string reason = SDL_GetError();
        close(); SDL_SetError("%s", reason.c_str()); return false;
    }
    static constexpr Uint32 kOutputSize = 704 * 512 * 4, kInputSize = 0x81200 + kOutputSize;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUComputePipeline* pipeline_ = nullptr;
    SDL_GPUBuffer *input_ = nullptr, *output_ = nullptr;
    SDL_GPUTransferBuffer *upload_ = nullptr, *download_ = nullptr;
};
