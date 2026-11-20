#include <crtdbg.h>
#include "sotn_bug_report.h"
#include "sotn_dev_report.h"
#include "sotn_gpu_shader.h"
#include <cassert>
#include <cstdio>


#include <cstdlib>
static SDL_Texture* headless_target = nullptr;
static bool setHeadlessTarget(SDL_Renderer* renderer, SDL_Texture* texture) {
    return SDL_SetRenderTarget(renderer, texture ? texture : headless_target);
}
#define SDL_SetRenderTarget setHeadlessTarget

int main(int argc, char** argv) {
    _set_error_mode(_OUT_TO_STDERR); _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    assert(argc == 2);
    namespace fs = std::filesystem;
    fs::path root(argv[1]);
    fs::create_directories(root);
    assert(SDL_Init(SDL_INIT_VIDEO));
    std::vector<uint32_t> pixels(64, 0x00808080);
    for (const char* backend : {"direct3d12", "vulkan"}) {
        auto session = root / backend;
        fs::create_directory(session);
        assert(SotnBugReport::text_file(session / "run.log", std::string(70000, 'x') + "shader failure test\n"));
        assert(SotnBugReport::text_file(session / "input.txt", "1:START,2:"));
        assert(SotnBugReport::text_file(session / "session.json", "{\"executable_sha256\":\"test-hash\"}"));
        assert(SotnBugReport::text_file(session / "backup.bin", "PERSONAL_SAVE_MUST_NOT_BE_COPIED"));
        auto format = SDL_strcmp(backend, "vulkan") == 0 ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_DXIL;
        auto* device = SDL_CreateGPUDevice(format, false, backend);
        assert(device);
        auto* renderer = SDL_CreateGPURenderer(device, nullptr);
        assert(renderer);
        headless_target=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,64,64);
        assert(headless_target && SDL_SetRenderTarget(renderer,headless_target));
        SotnGpuShader shader;
        assert(shader.open(renderer));
        auto* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STATIC, 8, 8);
        assert(texture && SDL_UpdateTexture(texture, nullptr, pixels.data(), 32));
        assert(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) && SDL_RenderClear(renderer));
        assert(shader.begin(1, 8, 8));
        assert(SDL_RenderTexture(renderer, texture, nullptr, nullptr));
        shader.end();
        auto result = SotnBugReport::capture(renderer, session, 9000, pixels.data(), 8, 8,
            {{"shader", "CRT"}, {"escaped", "quote\" slash\\ line\n"}}, "[display]\nshader=1\n");
        assert(result.created && result.rendered && result.original);
        assert(fs::file_size(result.folder / "run.log") == 65536);
        assert(fs::exists(result.folder / "input.txt") && !fs::exists(result.folder / "backup.bin"));
        assert(fs::exists(result.folder / "session.json"));
        std::string title;
        SotnDevReport::append(title, "Menu a\xc3\xa7", 160);
        SotnDevReport::backspace(title);
        assert(title == "Menu a");
        SotnDevReport::append(title, "123456", 7);
        assert(title == "Menu a");
        assert(SotnDevReport::save(result.folder, "Titulo \"bug\"", "Descricao\nlinha2"));
        assert(!SotnDevReport::save({}, "", ""));
        auto second = SotnBugReport::capture(nullptr, session, 9000, nullptr, 0, 0, {}, "");
        assert(second.created && !second.rendered && !second.original && second.folder != result.folder);
        shader.close();
        SDL_DestroyTexture(texture);
        assert(SDL_FlushRenderer(renderer) && SDL_WaitForGPUIdle(device));
        SDL_DestroyTexture(headless_target); headless_target=nullptr;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyGPUDevice(device);
        std::printf("PASS report %s\n", backend);
    }
    auto blocked = root / "blocked";
    assert(SotnBugReport::text_file(blocked, "file blocks directory creation"));
    auto failed = SotnBugReport::capture(nullptr, blocked, 1, nullptr, 0, 0, {}, "");
    assert(!failed.created && !failed.error.empty());
    SDL_Quit();
}
