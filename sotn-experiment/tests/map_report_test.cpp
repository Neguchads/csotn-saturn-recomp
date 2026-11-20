#include "sotn_map_overlay.h"
#include "sotn_report_layout.h"
#include "sotn_dev_layout.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <vector>

int main() {
    using namespace SotnMapOverlay;
    std::map<uint32_t,uint32_t> ram;
    auto b=[&](uint32_t a){return uint8_t(ram[a]);};
    auto w=[&](uint32_t a){return uint16_t(ram[a]);};
    auto d=[&](uint32_t a){return ram[a];};
    ram[0x0605CD70]=5;ram[0x0605D744]=1;ram[0x0605D750]=7;
    ram[0x0605CE68]=17;ram[0x0605CE6C]=36;
    ram[0x0605CD58]=320;ram[0x0605C5B4]=512;
    ram[0x0605CEC0]=0x6c; // descoberto, comprado, ambos, vazio.
    auto map=snapshot(b,w,d);
    assert(map.available && !map.inverted && map.player_x==18 && map.player_y==38);
    assert(map.cell(0,0)==1 && map.cell(1,0)==2 && map.cell(2,0)==3 && map.cell(3,0)==0);
    assert(map.cell(-1,0)==0 && map.cell(64,0)==0 && map.cell(0,64)==0);
    ram[0x0605D750]=0x27;ram[0x0605D2C0]=0x40;
    map=snapshot(b,w,d);
    assert(map.inverted && map.cell(0,0)==1 && map.player_x==45 && map.player_y==25);
    ram[0x0605CD70]=6;assert(!snapshot(b,w,d).available);
    ram[0x0605CD70]=5;ram[0x0605D744]=0;assert(!snapshot(b,w,d).available);
    ram[0x0605D744]=1;ram[0x0605D750]=0x1f;assert(!snapshot(b,w,d).available);
    Toggle toggle;
    assert(toggle.open()); // Visível ao iniciar; disponibilidade controla só o desenho.
    toggle.sync(false);assert(toggle.open());
    toggle.button(true,true);assert(!toggle.open());
    toggle.button(true,true);assert(!toggle.open());
    toggle.button(false,true);toggle.button(true,true);assert(toggle.open());
    toggle.toggle(true);toggle.sync(false);assert(!toggle.open());
    toggle.toggle(false);assert(!toggle.open());
    toggle.expand(true);assert(toggle.open() && toggle.fullscreen());
    toggle.pan(4,-3);assert(toggle.pan_x()==4 && toggle.pan_y()==-3);
    toggle.pan(1000,-1000);assert(toggle.pan_x()==63 && toggle.pan_y()==-63);
    toggle.close();assert(toggle.open() && !toggle.fullscreen());
    assert(toggle.pan_x()==0 && toggle.pan_y()==0);
    toggle.expand(true);assert(toggle.pan_x()==0 && toggle.pan_y()==0);
    Shortcut shortcut;
    assert(!shortcut.sample(true,false,100).toggle);
    auto combo=shortcut.sample(true,true,150);
    assert(combo.expand && combo.consume_cross && !combo.toggle);
    assert(!shortcut.sample(true,true,200).expand);
    shortcut.sample(false,false,210);
    assert(!shortcut.sample(true,false,300).toggle);
    assert(shortcut.sample(false,false,320).toggle);
    shortcut.sample(true,false,400);
    assert(shortcut.sample(true,false,580).toggle);
    assert(!shortcut.sample(true,false,600).toggle);
    shortcut.sample(false,false,620);
    combo=shortcut.sample(true,true,700);assert(combo.expand && combo.consume_cross);
    Snapshot markers;markers.available=true;
    markers.cells[30*16+7/4]=uint8_t(1<<(6-2*(7%4)));
    markers.cells[36*16+17/4]=uint8_t(1<<(6-2*(17%4)));
    markers.cells[38*16+15/4]=uint8_t(1<<(6-2*(15%4)));
    assert(markers.marker(7,30)==1 && markers.marker(15,38)==2);
    assert(markers.marker(17,36)==0); // Sala dos anjos/transição, não save.
    markers.cells.fill(0);assert(markers.marker(17,36)==0);

    for (const auto size : {std::pair{320,240},std::pair{1280,720},std::pair{1920,1080},std::pair{1080,1920}}) {
        const SDL_FRect viewport={10,20,float(size.first),float(size.second)};
        const auto minimap=SotnMapOverlay::panel(viewport);
        assert(minimap.x>viewport.x+viewport.w/2 && minimap.y>viewport.y);
        assert(minimap.x+minimap.w<viewport.x+viewport.w && minimap.y+minimap.h<viewport.y+viewport.h);
        assert(viewport.x+viewport.w-minimap.x-minimap.w<=std::max(2.f,viewport.h*.004f)+.1f);
        assert(minimap.w<=viewport.w*.281f && minimap.h<=viewport.h*.201f);
        const auto panel=SotnDevLayout::make(size.first,size.second,5,3);
        const auto form=SotnReportLayout::make({panel.panel.x,panel.panel.y,panel.panel.w,panel.panel.h},panel.title_y+65*panel.u,panel.u);
        for (const auto rect : {form.preview,form.title,form.description,form.capture,form.save,form.back}) {
            assert(rect.w>0 && rect.h>0);
            assert(rect.x>=panel.panel.x && rect.y>=panel.panel.y);
            assert(rect.x+rect.w<=panel.panel.x+panel.panel.w+.1f);
            assert(rect.y+rect.h<=panel.panel.y+panel.panel.h+.1f);
        }
        assert(form.title.y+form.title.h<form.description.y);
        assert(form.description.y+form.description.h<form.save.y);
    }
    // Renderer SDL real: a camada mantém o fundo e mistura só os quadrados.
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Surface* surface=SDL_CreateSurface(640,480,SDL_PIXELFORMAT_RGBA32);assert(surface);
    SDL_Renderer* renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
    map={};map.available=true;map.cells[0]=0x40;map.player_x=0;map.player_y=0;
    uint32_t untouched[2]{};
    for (int frame=0;frame<2;++frame) {
        SDL_SetRenderDrawColor(renderer,Uint8(20+frame*60),30,40,255);assert(SDL_RenderClear(renderer));
        draw(renderer,map,{0,0,640,480},0);assert(SDL_RenderPresent(renderer));
        auto* pixels=static_cast<uint32_t*>(surface->pixels);
        untouched[frame]=pixels[320+240*(surface->pitch/4)];
    }
    assert(untouched[0]!=untouched[1]);
    map=markers;map.available=true;map.player_x=11;map.player_y=34;
    map.cells[30*16+7/4]=uint8_t(1<<(6-2*(7%4)));
    map.cells[38*16+15/4]=uint8_t(1<<(6-2*(15%4)));
    SDL_SetRenderDrawColor(renderer,20,30,40,255);assert(SDL_RenderClear(renderer));
    draw(renderer,map,{0,0,640,480},0);assert(SDL_RenderPresent(renderer));
    int red=0,green=0;
    for (int y=0;y<480;++y) for (int x=0;x<640;++x) {
        Uint8 r,g,bv,a;
        const auto pixel=static_cast<uint32_t*>(surface->pixels)[y*(surface->pitch/4)+x];
        SDL_GetRGBA(pixel,SDL_GetPixelFormatDetails(surface->format),nullptr,&r,&g,&bv,&a);
        if (r>150 && r>g*2 && r>bv*2) ++red;
        if (g>150 && g>r*2 && g>bv) ++green;
    }
    assert(red>0 && green>0);
    assert(SDL_SavePNG(surface,"minimap-fixture.png"));
    std::vector<uint8_t> baseline;
    int shot=0;
    for (const auto offset:{std::pair{0.f,0.f},std::pair{2.f,0.f},std::pair{-2.f,0.f},std::pair{0.f,2.f},std::pair{0.f,-2.f}}) {
        SDL_SetRenderDrawColor(renderer,20,30,40,255);assert(SDL_RenderClear(renderer));
        draw(renderer,map,{0,0,640,480},0,true,offset.first,offset.second);
        assert(SDL_RenderPresent(renderer));
        const auto* begin=static_cast<uint8_t*>(surface->pixels);
        const std::vector<uint8_t> image(begin,begin+surface->pitch*surface->h);
        if (!shot) baseline=image; else assert(baseline!=image);
        char filename[64];std::snprintf(filename,sizeof filename,"fullscreen-direction-%d.png",shot++);
        assert(SDL_SavePNG(surface,filename));
    }
    SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_Quit();
    std::puts("PASS: live map data, controls, moving backdrop and report form layout");
}
