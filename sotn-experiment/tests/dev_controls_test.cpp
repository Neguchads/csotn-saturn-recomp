#include "sotn_dev_input.h"
#include "sotn_dev_speed.h"
#include "pace.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>

int main() {
    using SotnDevInput::MenuChord;
    // Start+R3 não abre mais o painel; Start continua sendo enviado ao jogo.
    MenuChord chord;
    assert(!chord.sample(false, true, 100).open_menu);
    auto sample = chord.sample(true, true, 110);
    assert(!sample.open_menu && !sample.guest_start && !chord.consumed());
    assert(!chord.sample(true, true, 500).open_menu);
    assert(chord.sample(true, false, 600).guest_start);
    assert(!chord.sample(true, true, 700).open_menu);
    assert(!chord.sample(false, false, 710).open_menu);
    assert(!chord.consumed());
    assert(!chord.sample(true, true, 720).open_menu);
    // Start -> R3 dentro da janela: Start não vaza para o menu original.
    MenuChord reverse;
    assert(!reverse.sample(true, false, 1000).guest_start);
    sample = reverse.sample(true, true, 1100);
    assert(!sample.open_menu && !sample.guest_start);
    // Start sozinho: toque curto preservado e botão segurado após a janela.
    MenuChord tap;
    assert(!tap.sample(true, false, 2000).guest_start);
    assert(tap.sample(false, false, 2030).guest_start);
    assert(tap.sample(false, false, 2040).guest_start);
    assert(!tap.sample(false, false, 2065).guest_start);
    MenuChord hold;
    assert(!hold.sample(true, false, 3000).guest_start);
    assert(hold.sample(true, false, 3180).guest_start);
    assert(!hold.sample(false, false, 3200).guest_start);
    // Controladores mantêm estados separados.
    MenuChord another;
    assert(!another.sample(false, false, 4000).open_menu);
    MenuChord sticks;
    assert(!sticks.sample(false,false,4100,true).open_menu);
    sample=sticks.sample(false,true,4110,true);
    assert(sample.open_menu && !sample.guest_start);
    assert(!sticks.sample(false,true,4120,true).open_menu);
    sticks.sample(false,false,4130,true);
    assert(!sticks.sample(false,true,4140,true).open_menu);
    sticks.sample(false,false,4150,false);
    assert(!sticks.sample(false,true,4160,false).open_menu);
    sample=sticks.sample(false,true,4170,true);
    assert(sample.open_menu && !sample.guest_start);

    SotnDevInput::MapShortcut map;
    assert(!map.request(10, false, false));
    assert(map.request(10, true, false));
    assert(!map.request(11, true, false));
    assert(map.buttons(10, false) == B_START);
    assert(map.buttons(13, false) == B_START);
    assert(map.buttons(14, true) == 0);
    assert(map.buttons(14, true) == 0);
    assert(map.buttons(15, false) == 0);
    assert(map.buttons(100, true) == B_L);
    assert(map.buttons(103, true) == B_L);
    assert(map.buttons(104, true) == 0 && !map.active());
    assert(map.request(200, true, true));
    assert(map.buttons(200, true) == B_L);
    assert(map.buttons(201, false) == 0 && !map.active());
    assert(map.request(300, true, false));
    assert(map.buttons(900, false) == 0 && !map.active());
    assert(map.request(1000, true, false));
    assert(map.buttons(999, true) == 0 && !map.active());
    assert(map.request(1100, true, false));
    map.cancel();
    assert(map.buttons(1100, true) == 0);

    using namespace std::chrono;
    // O mesmo mecanismo de pacing e cadence é exercitado em cada fator real.
    const auto now = PaceClock::time_point{} + seconds(10);
    for (unsigned speed : {1u, 2u, 4u, 6u, 8u, 10u}) {
        auto base = now;
        assert(pace_wait(base, 16000000, now, speed) == nanoseconds(16000000 / speed));
        unsigned presents = 0;
        for (uint64_t vblank = 1; vblank <= 600; ++vblank)
            if (SotnDevSpeed::should_present(vblank, false, speed)) ++presents;
        assert(presents == 600 / speed);
        assert(SotnDevSpeed::should_present(7, false, speed, true));
        SotnDevSpeed::Meter meter;
        assert(!meter.update(2000000000, 1000000000));
        assert(!meter.update(2000000000 + uint64_t(speed) * 500000000, 1500000000));
        assert(meter.update(2000000000 + uint64_t(speed) * 1000000000, 2000000000));
        assert(std::abs(meter.actual() - speed) < 0.000001);
    }
    assert(SotnDevSpeed::should_present(1, false, 0));
    assert(SotnDevSpeed::should_present(1, false, 7));
    assert(!SotnDevSpeed::should_present(29, true, 10));
    assert(SotnDevSpeed::should_present(30, true, 10));
    SotnDevSpeed::Meter stalled;
    assert(!stalled.update(0, 1000000000));
    assert(stalled.update(500000000, 2000000000));
    assert(stalled.actual() == 0.5);
    assert(!stalled.update(10, 2000000001));
    assert(stalled.actual() == 0);
    assert(!stalled.update(10, 1));
    std::puts("PASS: dev chord, normal Start, guarded map macro, render cadence and measured speed");
}
