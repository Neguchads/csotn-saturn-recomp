#include "video.h"
#include "sotn_vdp2_gpu.h"
#include <cstdio>
#include <stdexcept>
#define vdp2_compose cpu_vdp2_compose
#include "vdp2.cpp"
#undef vdp2_compose

extern SotnVdp2Gpu* test_gpu;
unsigned compared_fields = 0, fallback_fields = 0;
void vdp2_compose(Frame& output) {
    Vdp2GpuSnapshot snapshot;
    const char* reason = nullptr;
    const bool supported = vdp2_gpu_snapshot(snapshot, &reason);
    if (supported == (reason != nullptr)) throw std::runtime_error("GPU fallback reason contract failed");
    cpu_vdp2_compose(output);
    if (!supported) { ++fallback_fields; return; }
    Frame actual;
    if (!test_gpu->compose(snapshot, actual)) throw std::runtime_error(SDL_GetError());
    if (output.w != actual.w || output.h != actual.h || output.px != actual.px) {
        for (size_t i=0; i<output.px.size(); ++i) if(output.px[i]!=actual.px[i]) {
            std::fprintf(stderr,"GPU mismatch pixel %zu (%zux%zu): %08X != %08X\n",i,i%output.w,i/output.w,actual.px[i],output.px[i]);break;
        }
        throw std::runtime_error("GPU frame differs from CPU");
    }
    ++compared_fields;
}
