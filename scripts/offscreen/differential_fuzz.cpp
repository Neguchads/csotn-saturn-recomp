// Mede somente o compositor de produção, sem CPU do jogo, disco, janela ou save.
#include "video.h"
#include "sotn_vdp2_gpu.h"
#include <cstdlib>
static SotnVdp2Gpu gpu;
static bool use_gpu = false;
#include "state.h"
#include "saturn.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
void sat_note(const char*, ...) {}
[[noreturn]] void state_fail(const char* message) { throw std::runtime_error(message); }
// Dependências da TU VDP1 que o teste não executa: falhar se forem alcançadas.
SaturnConfig g_cfg;
SH2Context* g_cpu = nullptr;
void scu_raise(int) { state_fail("unexpected interrupt"); }
void scu_frame_event(int) { state_fail("unexpected frame event"); }
void sat_trace(const char*, ...) { state_fail("unexpected traced operation"); }
uint64_t sat_now() { state_fail("unexpected machine clock"); }
uint64_t onchip_phi_ns(uint64_t) { state_fail("unexpected draw clock"); }
[[noreturn]] void sat_fatal(const char*, ...) { state_fail("unexpected fatal"); }
#include "vdp1.cpp"


static uint32_t random_state=0x1026C618u;
static uint32_t next_random(){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static void set_reg(unsigned a,uint16_t value){g_vdp2_regs[a]=uint8_t(value>>8);g_vdp2_regs[a+1]=uint8_t(value);}
int main(int argc,char** argv) {
 if(argc!=2||!SDL_Init(SDL_INIT_VIDEO)||!gpu.open(argv[1]))return 2;
 for(unsigned c=0;c<96;++c) {
  const auto initial_seed=random_state;
  vdp2_reset_fetch();
  for(auto& v:g_vdp2_vram)v=uint8_t(next_random());
  for(auto& v:g_vdp2_cram)v=uint8_t(next_random());
  for(unsigned a=0;a<sizeof g_vdp2_regs;a+=2)set_reg(a,uint16_t(next_random()));
  set_reg(0,uint16_t(0x8000|(c%4)|((c/4%3)<<4)|((c/12%2)?0xC0:0)));
  set_reg(0x20,uint16_t(15|(next_random()&0x0F00)));
  set_reg(0x9A,uint16_t(next_random()&0x3E3E)); // Line/mosaic supported, no vertical-cell.
  set_reg(0xEC,uint16_t(next_random()&0x7FFF)); // No sequential gradation.
  if(c%3==0){set_reg(0xD0,0);set_reg(0xD2,0);set_reg(0xD4,0);set_reg(0xD6,0);}
  set_reg(0xF8,uint16_t(0x0101+(next_random()&0x0606)));
  set_reg(0xFA,uint16_t(0x0101+(next_random()&0x0606)));
  std::memset(g_reg,0,sizeof g_reg);
  for(unsigned i=0;i<512*256;++i)g_fb[0][i]=g_fb[1][i]=uint16_t(next_random());
  g_hi_on[0]=g_hi_on[1]=false;g_show_ifb=false;g_draw_fb=0;g_target=g_fb[0];g_hi_target=nullptr;
  Frame expected,actual;Vdp2GpuSnapshot snapshot;
  if(!vdp2_gpu_snapshot(snapshot)){std::fprintf(stderr,"unexpected fallback case=%u\n",c);return 3;}
  vdp2_compose(expected);
  if(!gpu.compose(snapshot,actual)){std::fprintf(stderr,"GPU failed case=%u %s\n",c,SDL_GetError());return 4;}
  if(expected.w!=actual.w||expected.h!=actual.h||expected.px!=actual.px){
   size_t first=0;while(first<expected.px.size()&&first<actual.px.size()&&expected.px[first]==actual.px[first])++first;
   std::fprintf(stderr,"MISMATCH case=%u seed=%08X pixel=%zu size=%dx%d actual=%08X expected=%08X\n",c,initial_seed,first,expected.w,expected.h,actual.px.at(first),expected.px.at(first));return 5;
  }
  std::printf("PASS case=%u seed=%08X size=%dx%d\n",c,initial_seed,expected.w,expected.h);std::fflush(stdout);
 }
 gpu.close();SDL_Quit();std::puts("PASS: 96 deterministic supported graphics combinations, all pixels identical");
}
