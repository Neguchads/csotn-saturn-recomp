#include "sotn_fsr.h"
#include <cassert>
#include <vector>
#include <cstdio>

static unsigned channel(SDL_Surface* surface,int x,int y) {
    Uint8 r,g,b,a;
    assert(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a));
    return r;
}
int main() {
    assert(SDL_Init(SDL_INIT_VIDEO));
    auto* window=SDL_CreateWindow("FSR1 verification",64,64,SDL_WINDOW_HIDDEN);
    assert(window);
    for (const char* driver : {"direct3d12","vulkan"}) {
        auto* device=SDL_CreateGPUDevice(SDL_strcmp(driver,"vulkan")==0 ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_DXIL,false,driver);
        assert(device);
        auto* renderer=SDL_CreateGPURenderer(device,window);
        assert(renderer);
        SotnFsr fsr;
        if(!fsr.open(renderer)) { std::fprintf(stderr,"%s\n",fsr.status()); return 2; }
        auto* source=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,8,8);
        std::vector<Uint32> pixels(64,0xFF808080);
        assert(source && SDL_UpdateTexture(source,nullptr,pixels.data(),32));
        auto* result=fsr.render(source,32,32,0.2f);
        assert(result && result!=source && result->w==32 && result->h==32);
        assert(SDL_SetRenderTarget(renderer,result));
        auto* uniform=SDL_RenderReadPixels(renderer,nullptr);
        if(!uniform) { std::fprintf(stderr,"%s\n",SDL_GetError()); return 3; }
        assert(channel(uniform,16,16)>=125 && channel(uniform,16,16)<=130);
        SDL_DestroySurface(uniform);
        assert(SDL_SetRenderTarget(renderer,nullptr));
        for(int y=0;y<8;++y) for(int x=0;x<8;++x) pixels[size_t(y*8+x)]=x>y ? 0xFFFFFFFF : 0xFF000000;
        assert(SDL_UpdateTexture(source,nullptr,pixels.data(),32));
        result=fsr.render(source,32,32,0.2f);
        assert(SDL_SetRenderTarget(renderer,result));
        auto* edge=SDL_RenderReadPixels(renderer,nullptr);
        assert(edge);
        assert(channel(edge,28,3)>245 && channel(edge,3,28)<10);
        unsigned intermediate=0;
        for(int y=3;y<29;++y) for(int x=3;x<29;++x) { const auto r=channel(edge,x,y); if(r>0 && r<255) ++intermediate; }
        assert(intermediate>0);
        SDL_DestroySurface(edge);
        assert(SDL_SetRenderTarget(renderer,nullptr));
        assert(fsr.render(source,0,0,0.2f)==source);
        assert(fsr.render(source,4,4,0.2f)==source);
        fsr.close();
        SDL_DestroyTexture(source);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyGPUDevice(device);
        std::printf("PASS: FSR1 EASU RCAS pixels on %s\n",driver);
    }
    SDL_DestroyWindow(window); SDL_Quit();
}
