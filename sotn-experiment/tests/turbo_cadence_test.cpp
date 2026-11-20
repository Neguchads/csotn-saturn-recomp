#include "sotn_dev_speed.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <cstdlib>

constexpr uint64_t period = 16715228;
int main() {
#if defined(_WIN32)
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    // O relógio é fixo. Só a decisão de desenhar varia; todos os VB são visitados.
    for (unsigned requested : {2u,4u,6u,8u,10u}) {
        for (unsigned actual_vbs : {45u,60u,91u,104u,300u,598u}) {
            SotnDevSpeed::Presentation gate;
            unsigned new_frames=0,old_frames=0;
            for (uint64_t vb=1;vb<=uint64_t(actual_vbs)*10;++vb) {
                const uint64_t ns=(vb-1)*1000000000/actual_vbs;
                if (gate.wants(vb,false,requested,false,ns,period)) ++new_frames;
                if (SotnDevSpeed::should_present(vb,false,requested)) ++old_frames;
            }
            const unsigned expected=actual_vbs<60?actual_vbs*10:598;
            assert(new_frames+1>=expected&&new_frames<=expected+1);
            if (requested>=6&&actual_vbs==91) assert(new_frames>=old_frames*3);
            if (requested>=6&&actual_vbs==104) assert(new_frames>=old_frames*3);
            std::printf("requested=%u actual_vbs=%u legacy_fps=%.3f adaptive_fps=%.3f\n",requested,actual_vbs,old_frames/10.0,new_frames/10.0);
        }
    }
    SotnDevSpeed::Presentation gate;
    for (uint64_t vb=1;vb<200;++vb) assert(gate.wants(vb,false,1,false,vb*1000,period));
    assert(gate.wants(200,false,0,false,0,period));
    assert(gate.wants(201,false,7,false,0,period));
    assert(!gate.wants(29,true,10,false,0,period));
    assert(gate.wants(30,true,10,false,0,period));
    assert(gate.wants(31,true,10,true,0,period));
    gate.reset();
    assert(gate.wants(1,false,10,false,1000000000,period));
    assert(!gate.wants(2,false,10,false,1000001000,period));
    assert(gate.wants(3,false,10,true,1000002000,period));
    assert(gate.wants(4,false,6,false,1000003000,period)); // troca reinicia a janela
    assert(gate.wants(5,false,6,false,999999000,period)); // relógio recuou
    assert(gate.wants(6,false,6,false,8000000000,period)); // stall não gera rajada
    assert(!gate.wants(7,false,6,false,8000000001,period));
    gate.reset(); assert(gate.wants(1,false,10,false,0,period));
    const auto late = period*101-1;
    assert(gate.wants(2,false,10,false,late,period));
    assert(!gate.wants(3,false,10,false,late+1,period)); // não recuperar o prazo antigo em rajada
    assert(gate.wants(4,false,10,false,late+period,period));
    gate.reset(); assert(gate.wants(8,false,6,false,8000000002,period));
    // Subtração segura perto do limite uint64, sem construir deadline que estoure.
    gate.reset();
    assert(gate.wants(9,false,10,false,UINT64_MAX-period*2,period));
    assert(gate.wants(10,false,10,false,UINT64_MAX-period,period));
    assert(gate.wants(11,false,10,false,UINT64_MAX,period));
    std::puts("PASS: adaptive turbo presentation, normal cadence, force, resume and stalls");
}
