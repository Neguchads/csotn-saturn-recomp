// Reutiliza o oraculo de pixels HD ja exercitado no D3D12, sem duplicar regras.
#define main d3d12_shader_probe_main
#include "gpu_shader_test.cpp"
#undef main

int main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window* window = SDL_CreateWindow("SotN Vulkan shader probe", 64, 64, SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    if (!device) { std::fprintf(stderr, "Vulkan device: %s\n", SDL_GetError()); return 2; }
    assert(SDL_strcmp(SDL_GetGPUDeviceDriver(device), "vulkan") == 0);
    SDL_Renderer* renderer = SDL_CreateGPURenderer(device, window);
    if (!renderer) { std::fprintf(stderr, "Vulkan renderer: %s\n", SDL_GetError()); return 3; }
    SotnGpuShader shader;
    if (!shader.open(renderer)) { std::fprintf(stderr, "%s\n", shader.status()); return 4; }
    assert(shader.enabled() && SDL_strstr(shader.status(), "vulkan") && SDL_strstr(shader.status(), "SPIR-V"));
    std::fprintf(stderr, "%s\n", shader.status());
    std::array<Uint32, 64> pixels;
    pixels.fill(0xFFFFFFFFu);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 8, 8);
    assert(texture && SDL_UpdateTexture(texture, nullptr, pixels.data(), 8 * sizeof(Uint32)));
    for (int mode : {0, 1, 2}) {
        assert(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) && SDL_RenderClear(renderer));
        assert(shader.begin(mode, 8, 8) && SDL_RenderTexture(renderer, texture, nullptr, nullptr));
        shader.end();
        assert(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
        const SDL_FRect ui{56, 56, 8, 8};
        assert(SDL_RenderFillRect(renderer, &ui));
        SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
        if (!surface) { std::fprintf(stderr, "Vulkan readback: %s\n", SDL_GetError()); return 5; }
        const Pixel a = pixel(surface, 4, 28), b = pixel(surface, 12, 28), c = pixel(surface, 20, 28);
        const Pixel clean = pixel(surface, 60, 60);
        assert(clean.r >= 254 && clean.g >= 254 && clean.b >= 254);
        if (mode == 0) assert(a.r >= 254 && a.g >= 254 && a.b >= 254);
        if (mode == 1) {
            const Pixel even = pixel(surface, 28, 36), odd = pixel(surface, 28, 28);
            assert(even.r > odd.r + 25 && odd.r > 100);
            std::fprintf(stderr, "Vulkan CRT even=%u odd=%u\n", even.r, odd.r);
        }
        if (mode == 2) {
            assert(a.r > a.g + 20 && a.r > a.b + 20);
            assert(b.g > b.r + 20 && b.g > b.b + 20);
            assert(c.b > c.r + 20 && c.b > c.g + 20);
        }
        SDL_DestroySurface(surface);
        assert(SDL_RenderPresent(renderer));
    }
    shader.disable("Teste Vulkan desligado");
    assert(!shader.enabled() && SDL_strcmp(shader.status(), "Teste Vulkan desligado") == 0);
    testHD(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: real Vulkan GPU CRT/LCD, HD 2x/4x/6x/8x and clean UI");
}
