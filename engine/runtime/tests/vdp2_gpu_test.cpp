#include "test.h"
#include "video.h"
#include "state.h"
#include "saturn.h"
#include "sotn_vdp2_gpu.h"
#include <stdexcept>
#include <cstring>

void sat_note(const char*, ...) {}
[[noreturn]] void state_fail(const char* message) { throw std::runtime_error(message); }
SaturnConfig g_cfg;
SH2Context* g_cpu = nullptr;
void scu_raise(int) { state_fail("unexpected interrupt"); }
void scu_frame_event(int) { state_fail("unexpected frame event"); }
// Os testes gráficos escrevem registradores e geram logs de VDP1 normalmente.
void sat_trace(const char*, ...) {}
uint64_t sat_now() { state_fail("unexpected machine clock"); }
uint64_t onchip_phi_ns(uint64_t) { state_fail("unexpected draw clock"); }
[[noreturn]] void sat_fatal(const char*, ...) { state_fail("unexpected fatal"); }
#include "vdp1.cpp"

static Test* tests = nullptr;
static unsigned failures = 0;
Test::Test(const char* n,void(*f)()) : name(n),fn(f),next(tests) {tests=this;}
void check_failed(const char* file,int line,const char* expression,uint64_t got,uint64_t want) {
    ++failures; std::fprintf(stderr,"%s:%d %s %llX != %llX\n",file,line,expression,(unsigned long long)got,(unsigned long long)want);
}
SotnVdp2Gpu* test_gpu = nullptr;
extern unsigned compared_fields, fallback_fields;
int main(int argc,char** argv) {
    if(argc!=2 || !SDL_Init(SDL_INIT_VIDEO)) return 2;
    SotnVdp2Gpu gpu;
    if(!gpu.open(argv[1])) {std::fprintf(stderr,"%s\n",SDL_GetError());return 3;}
    test_gpu=&gpu;
    unsigned count=0;
    for(Test* t=tests;t;t=t->next) {
        const unsigned before = compared_fields, fallback_before = fallback_fields;
        try { t->fn(); }
        catch(const std::exception& error) {++failures;std::fprintf(stderr,"FAIL %s: %s\n",t->name,error.what());break;}
        const char* required[] = {
            "mosaic_and_packed_line_scroll_keep_full_density_coordinates",
            "line_horizontal_scroll_obeys_each_table_interval_and_scroll_register",
            "line_vertical_scroll_restarts_its_accumulator_in_each_interval",
            "line_scroll_and_zoom_use_only_enabled_fields_in_packed_tables",
            "nbg1_has_an_independent_line_scroll_table",
            "mosaic_replicates_screen_groups_before_scroll_and_fractional_zoom",
            "double_density_line_scroll_uses_full_output_line_indices_in_the_snapshot_model"};
        for (const char* name : required) if (std::strcmp(t->name, name) == 0) {
            if (compared_fields == before || fallback_fields != fallback_before) {
                ++failures; std::fprintf(stderr,"FAIL %s: tested feature fell back to CPU\n",t->name);
            }
        }
        ++count;
    }
    std::printf("%s: %u tests, %u failures, %u GPU fields compared, %u CPU fallback fields\n",gpu.backend(),count,failures,compared_fields,fallback_fields);
    gpu.close();SDL_Quit();return failures?1:0;
}
