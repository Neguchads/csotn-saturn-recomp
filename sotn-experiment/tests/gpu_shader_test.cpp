#include "sotn_gpu_shader.h"
#include "sotn_hd_scaler.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdio>
#include <vector>

struct Pixel { Uint8 r, g, b, a; };

static Pixel pixel(SDL_Surface* surface, int x, int y) {
    Pixel result{};
    assert(SDL_ReadSurfacePixel(surface, x, y, &result.r, &result.g, &result.b, &result.a));
    return result;
}

// Oraculo CPU independente: regras de vizinhanca aplicadas pixel por pixel.
static std::vector<Uint32> scaleReference(const std::vector<Uint32>& pixels, int w, int h, int scale) {
    std::vector<Uint32> output(static_cast<size_t>(w * h * scale * scale));
    auto read = [&](int x, int y) { return pixels[static_cast<size_t>(std::clamp(y, 0, h - 1) * w + std::clamp(x, 0, w - 1))]; };
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        const Uint32 a = read(x - 1, y - 1), b = read(x, y - 1), c = read(x + 1, y - 1);
        const Uint32 d = read(x - 1, y), e = read(x, y), f = read(x + 1, y);
        const Uint32 g = read(x - 1, y + 1), i = read(x + 1, y + 1), below = read(x, y + 1);
        for (int v = 0; v < scale; ++v) for (int u = 0; u < scale; ++u) {
            Uint32 value = e;
            if (b != below && d != f) {
                if (scale == 2) {
                    const Uint32 vertical = v == 0 ? b : below;
                    const Uint32 horizontal = u == 0 ? d : f;
                    if (vertical == horizontal) value = horizontal;
                } else if (v == 0 && u == 0 && d == b) value = d;
                else if (v == 0 && u == 1 && ((d == b && e != c) || (b == f && e != a))) value = b;
                else if (v == 0 && u == 2 && b == f) value = f;
                else if (v == 1 && u == 0 && ((d == b && e != g) || (d == below && e != a))) value = d;
                else if (v == 1 && u == 2 && ((b == f && e != i) || (below == f && e != c))) value = f;
                else if (v == 2 && u == 0 && d == below) value = d;
                else if (v == 2 && u == 1 && ((d == below && e != i) || (below == f && e != g))) value = below;
                else if (v == 2 && u == 2 && below == f) value = f;
            }
            output[static_cast<size_t>((y * scale + v) * w * scale + x * scale + u)] = value;
        }
    }
    return output;
}

static void testHD(SDL_Renderer* renderer) {
    SotnHdScaler scaler;
    assert(scaler.open(renderer));
    const std::vector<Uint32> input{
        0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000,
    };
    SDL_Texture* source = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 5, 5);
    SDL_Texture* previous = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 8, 8);
    assert(source && previous && SDL_UpdateTexture(source, nullptr, input.data(), 5 * sizeof(Uint32)));
    assert(SDL_SetTextureScaleMode(source, SDL_SCALEMODE_LINEAR));
    for (int factor : {2, 4, 6, 8}) {
        assert(SDL_UpdateTexture(source, nullptr, input.data(), 5 * sizeof(Uint32)));
        assert(scaler.resize(5, 5, factor) && scaler.factor() == factor);
        assert(scaler.resize(5, 5, factor)); // Reutiliza alvos quando a configuracao nao mudou.
        assert(SDL_SetRenderTarget(renderer, previous));
        SDL_Texture* output = scaler.render(source);
        assert(output && output != source && output->w == 5 * factor && output->h == 5 * factor);
        assert(SDL_GetRenderTarget(renderer) == previous);
        SDL_ScaleMode restored;
        assert(SDL_GetTextureScaleMode(source, &restored) && restored == SDL_SCALEMODE_LINEAR);
        std::vector<Uint32> expected = scaleReference(input, 5, 5, 2);
        int size = 10;
        if (factor > 2) { expected = scaleReference(expected, size, size, factor == 6 ? 3 : 2); size *= factor == 6 ? 3 : 2; }
        if (factor == 8) { expected = scaleReference(expected, size, size, 2); size *= 2; }
        assert(SDL_SetRenderTarget(renderer, output));
        SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
        assert(surface);
        int changed = 0;
        for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
            const Pixel actual = pixel(surface, x, y);
            const Uint32 want = expected[static_cast<size_t>(y * size + x)];
            assert(actual.r == ((want >> 16) & 255) && actual.g == ((want >> 8) & 255) && actual.b == (want & 255));
            if (want != input[static_cast<size_t>((y / factor) * 5 + x / factor)]) ++changed;
        }
        assert(changed > 0); // Prova melhora de contorno, nao so nearest aumentado.
        std::fprintf(stderr, "HD %dx: %dx%d, edge pixels changed=%d\n", factor, size, size, changed);
        SDL_DestroySurface(surface);
        assert(SDL_SetRenderTarget(renderer, nullptr));
        const std::vector<Uint32> changedFrame(25, 0xFFCC2244u);
        assert(SDL_UpdateTexture(source, nullptr, changedFrame.data(), 5 * sizeof(Uint32)));
        output = scaler.render(source);
        assert(output && output != source && SDL_SetRenderTarget(renderer, output));
        surface = SDL_RenderReadPixels(renderer, nullptr);
        assert(surface);
        for (int coordinate : {0, size / 2, size - 1}) {
            const Pixel fresh = pixel(surface, coordinate, coordinate);
            assert(fresh.r == 204 && fresh.g == 34 && fresh.b == 68);
        }
        SDL_DestroySurface(surface);
        assert(SDL_SetRenderTarget(renderer, nullptr));
    }
    assert(scaler.resize(5, 5, 1) && scaler.factor() == 1 && scaler.render(source) == source);
    assert(!scaler.resize(INT_MAX, INT_MAX, 8) && scaler.factor() == 1);
    assert(!scaler.resize(0, 5, 2) && !scaler.resize(5, 5, 3));
    assert(scaler.resize(5, 5, 2));
    SDL_Texture* wrong = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
    assert(wrong && scaler.render(wrong) == wrong && scaler.factor() == 1);
    scaler.close();
    assert(scaler.factor() == 1 && scaler.render(source) == source);
    SDL_DestroyTexture(wrong);
    SDL_DestroyTexture(previous);
    SDL_DestroyTexture(source);
}

static void benchmarkHD(SDL_Renderer* renderer) {
    SotnHdScaler scaler;
    assert(scaler.open(renderer));
    std::vector<Uint32> frame(320 * 224);
    for (int y = 0; y < 224; ++y) for (int x = 0; x < 320; ++x) {
        frame[static_cast<size_t>(y * 320 + x)] = (x + y) % 7 < 3 ? 0xFFFFFFFFu : 0xFF103090u;
    }
    SDL_Texture* source = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 320, 224);
    assert(source && SDL_UpdateTexture(source, nullptr, frame.data(), 320 * sizeof(Uint32)));
    for (int factor : {1, 2, 4, 6, 8}) {
        assert(scaler.resize(320, 224, factor));
        const auto started = std::chrono::steady_clock::now();
        for (int repeat = 0; repeat < 20; ++repeat) {
            SDL_Texture* output = scaler.render(source);
            assert(output && SDL_RenderTexture(renderer, output, nullptr, nullptr));
            // Readback pequeno sincroniza a GPU: nao mede somente a fila de comandos.
            SDL_Surface* check = SDL_RenderReadPixels(renderer, nullptr);
            assert(check);
            SDL_DestroySurface(check);
            assert(SDL_RenderPresent(renderer));
        }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count() / 20;
        std::fprintf(stderr, "HD benchmark %dx 320x224: %.3f ms/frame (GPU + readback + present, debug on)\n", factor, ms);
    }
    scaler.close();
    SDL_DestroyTexture(source);
}

int main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetHint(SDL_HINT_GPU_DRIVER, "direct3d12");
    SDL_SetHint(SDL_HINT_RENDER_GPU_DEBUG, "1");
    SDL_Window* window = SDL_CreateWindow("SotN shader probe", 64, 64, SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_PropertiesID properties = SDL_CreateProperties();
    SDL_SetStringProperty(properties, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_GPU_RENDERER);
    SDL_SetPointerProperty(properties, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);
    SDL_SetBooleanProperty(properties, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true);
    SDL_Renderer* renderer = SDL_CreateRendererWithProperties(properties);
    SDL_DestroyProperties(properties);
    if (!renderer) { std::fprintf(stderr, "GPU renderer: %s\n", SDL_GetError()); return 2; }
    SotnGpuShader shader;
    if (!shader.open(renderer)) { std::fprintf(stderr, "%s\n", shader.status()); return 3; }
    assert(shader.enabled());
    std::array<Uint32, 64> white;
    white.fill(0xFFFFFFFFu);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 8, 8);
    assert(texture && SDL_UpdateTexture(texture, nullptr, white.data(), 8 * sizeof(Uint32)));
    assert(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST));
    for (int mode : {0, 0, 1, 2}) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        assert(shader.begin(mode, 8, 8));
        assert(SDL_RenderTexture(renderer, texture, nullptr, nullptr));
        shader.end();
        // O desenho seguinte representa a UI e deve permanecer branco sem efeito.
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        const SDL_FRect ui{56, 56, 8, 8};
        assert(SDL_RenderFillRect(renderer, &ui));
        SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
        std::fprintf(stderr, "Mode %d SDL error: %s\n", mode, SDL_GetError());
        if (!surface) { std::fprintf(stderr, "Readback: %s\n", SDL_GetError()); return 4; }
        const Pixel a = pixel(surface, 4, 28);
        const Pixel b = pixel(surface, 12, 28);
        const Pixel c = pixel(surface, 20, 28);
        const Pixel clean = pixel(surface, 60, 60);
        assert(clean.r >= 254 && clean.g >= 254 && clean.b >= 254);
        if (mode == 0) assert(a.r >= 254 && a.g >= 254 && a.b >= 254);
        if (mode == 1) {
            const Pixel even = pixel(surface, 28, 36);
            const Pixel odd = pixel(surface, 28, 28);
            std::fprintf(stderr, "CRT even=%u odd=%u\n", even.r, odd.r);
            assert(even.r > odd.r + 25 && odd.r > 100);
        }
        if (mode == 2) {
            assert(a.r > a.g + 20 && a.r > a.b + 20);
            assert(b.g > b.r + 20 && b.g > b.b + 20);
            assert(c.b > c.r + 20 && c.b > c.g + 20);
        }
        SDL_DestroySurface(surface);
        assert(SDL_RenderPresent(renderer));
    }
    assert(!shader.begin(5, 8, 8));
    assert(!shader.begin(1, 0, 8));
    shader.disable("Shader desativado por falha no driver");
    assert(!shader.enabled());
    assert(SDL_strcmp(shader.status(), "Shader desativado por falha no driver") == 0);
    assert(shader.open(renderer) && shader.enabled());
    shader.disable(nullptr);
    assert(!shader.enabled() && SDL_strcmp(shader.status(), "Shader GPU desativado") == 0);
    testHD(renderer);
    benchmarkHD(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    // O backend anterior continua válido e rejeita shaders com status explícito.
    renderer = SDL_CreateRenderer(window, "direct3d11");
    assert(renderer && !shader.open(renderer) && !shader.enabled());
    SotnHdScaler unavailable;
    assert(!unavailable.open(renderer) && !unavailable.resize(5, 5, 2));
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("PASS: real D3D12 GPU CRT/LCD pixels, clean UI and D3D11 fallback");
    return 0;
}
