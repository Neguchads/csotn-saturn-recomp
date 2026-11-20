#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include "sotn_map_markers.h"

namespace SotnMapOverlay {
// Leitura do mapa explorado do Saturn JP. Nunca escreve no estado do jogo.
struct Snapshot {
    bool available = false, inverted = false;
    int player_x = -1, player_y = -1;
    std::array<uint8_t, 1024> cells{};
    uint8_t cell(int x, int y) const {
        if (x < 0 || y < 0 || x >= 64 || y >= 64) return 0;
        return (cells[size_t(y)*16 + size_t(x)/4] >> (6-2*(x%4))) & 3;
    }
    int marker(int x,int y) const {
        if (!cell(x,y)) return 0;
        for (const auto& marker:kMarkers)
            if (marker.inverted==inverted && marker.x==x && marker.y==y) return marker.kind;
        return 0;
    }
};

template<class Read8, class Read16, class Read32>
Snapshot snapshot(Read8 byte, Read16 word, Read32 dword) {
    Snapshot result;
    const auto stage = word(0x0605D750);
    result.available = word(0x0605CD70) == 5 && word(0x0605D744) != 0 &&
                       dword(0x0605C6D0) <= 2 && (stage & 0x1f) != 0x1f;
    if (!result.available) return result;
    result.inverted = (stage & 0x20) != 0;
    const uint32_t base = 0x0605CEC0 + (result.inverted ? 0x400 : 0);
    for (size_t i=0; i<result.cells.size(); ++i) result.cells[i] = byte(base+uint32_t(i));
    result.player_x = int32_t(dword(0x0605CE68)) + int32_t(dword(0x0605CD58))/320;
    result.player_y = int32_t(dword(0x0605CE6C)) + int32_t(dword(0x0605C5B4))/256;
    if (result.inverted) {
        result.player_x = 63-result.player_x;
        result.player_y = 63-result.player_y;
    }
    return result;
}

class Toggle {
public:
    void sync(bool) {} // Ocultar durante menus não altera a preferência da sessão.
    void toggle(bool available) { if (available) open_ = !open_; }
    void button(bool down, bool available) {
        sync(available);
        if (down && !held_) toggle(available);
        held_ = down;
    }
    void close() {
        if (full_) { full_=false; pan_x_=pan_y_=0; }
        else open_ = false;
    }
    bool open() const { return open_; }
    bool fullscreen() const { return full_; }
    void expand(bool available) {
        if (!available) return;
        full_=!full_; open_=true; pan_x_=pan_y_=0;
    }
    void pan(float x,float y) {
        pan_x_=std::clamp(pan_x_+x,-63.f,63.f);
        pan_y_=std::clamp(pan_y_+y,-63.f,63.f);
    }
    float pan_x() const { return pan_x_; }
    float pan_y() const { return pan_y_; }
private:
    bool open_ = true, held_ = false;
    bool full_ = false;
    float pan_x_=0,pan_y_=0;
};

// Select sozinho aguarda o X; a combinação não vaza pulo para o jogo.
class Shortcut {
public:
    struct Action { bool toggle=false, expand=false, consume_cross=false; };
    Action sample(bool select,bool cross,Uint64 now) {
        Action result;
        result.consume_cross=select || consumed_;
        if (consumed_) { if (!select && !cross) consumed_=false; previous_=select; return result; }
        if (select && !previous_) { since_=now; pending_=true; }
        if (select && cross) {
            result.expand=true; pending_=false; consumed_=true;
        } else if (pending_ && (!select || now-since_>=180)) {
            result.toggle=true; pending_=false;
        }
        previous_=select;
        return result;
    }
private:
    bool previous_=false,pending_=false,consumed_=false;
    Uint64 since_=0;
};

inline SDL_FRect panel(const SDL_FRect& viewport) {
    const float margin=std::max(2.f,std::min(viewport.w,viewport.h)*.004f);
    const float width=viewport.w*.28f, height=viewport.h*.20f;
    return {viewport.x+viewport.w-margin-width,viewport.y+margin,width,height};
}

inline void draw(SDL_Renderer* renderer, const Snapshot& map, const SDL_FRect& viewport, Uint64 ticks,
                 bool fullscreen=false,float pan_x=0,float pan_y=0) {
    if (!map.available) return;
    const float unit = std::min(viewport.w*.96f/320, viewport.h*.95f/256)*.90f*(fullscreen ? 1.8f : 1.f);
    const float cw=5*unit, ch=4*unit;
    const auto box=fullscreen ? SDL_FRect{viewport.x+4,viewport.y+4,viewport.w-8,viewport.h-8} : panel(viewport);
    const auto centre=[](float position,float half) { return half>=32 ? 32.f : std::clamp(position,half,64-half); };
    // A visão ampliada permite navegar também além da borda do castelo.
    // Limitar a posição antes do pan criava uma zona morta nas extremidades.
    const float centre_x=fullscreen ? float(map.player_x)+.5f+pan_x : centre(float(map.player_x)+.5f,box.w/(2*cw));
    const float centre_y=fullscreen ? float(map.player_y)+.5f+pan_y : centre(float(map.player_y)+.5f,box.h/(2*ch));
    const float ox=box.x+box.w/2-centre_x*cw, oy=box.y+box.h/2-centre_y*ch;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,8,10,22,fullscreen ? 215 : 110);
    SDL_RenderFillRect(renderer,&box);
    SDL_Rect previous{};
    const bool clipped=SDL_RenderClipEnabled(renderer);
    SDL_GetRenderClipRect(renderer,&previous);
    const SDL_Rect clip={int(box.x+2),int(box.y+2),int(box.w-4),int(box.h-4)};
    SDL_SetRenderClipRect(renderer,&clip);
    for (int y=0; y<64; ++y) for (int x=0; x<64; ++x) {
        const auto cell=map.cell(x,y);
        if (!cell) continue;
        SDL_FRect rect={ox+x*cw,oy+y*ch,cw,ch};
        // Não enviar à GPU células que o recorte descartará por inteiro.
        // A margem conserva as linhas da borda e o arredondamento de coordenadas.
        if (rect.x+cw<clip.x-2 || rect.y+ch<clip.y-2 ||
            rect.x>clip.x+clip.w+2 || rect.y>clip.y+clip.h+2) continue;
        if (map.marker(x,y)==1) SDL_SetRenderDrawColor(renderer,235,50,60,230);
        else if (map.marker(x,y)==2) SDL_SetRenderDrawColor(renderer,45,220,110,230);
        else if (cell & 1) SDL_SetRenderDrawColor(renderer,55,69,224,165);
        else SDL_SetRenderDrawColor(renderer,130,135,160,110);
        SDL_RenderFillRect(renderer,&rect);
        SDL_SetRenderDrawColor(renderer,215,222,255,220);
        if (!map.cell(x-1,y)) SDL_RenderLine(renderer,rect.x,rect.y,rect.x,rect.y+ch);
        if (!map.cell(x+1,y)) SDL_RenderLine(renderer,rect.x+cw,rect.y,rect.x+cw,rect.y+ch);
        if (!map.cell(x,y-1)) SDL_RenderLine(renderer,rect.x,rect.y,rect.x+cw,rect.y);
        if (!map.cell(x,y+1)) SDL_RenderLine(renderer,rect.x,rect.y+ch,rect.x+cw,rect.y+ch);
    }
    if (map.player_x>=0 && map.player_x<64 && map.player_y>=0 && map.player_y<64) {
        SDL_FRect player={ox+map.player_x*cw,oy+map.player_y*ch,cw,ch};
        SDL_SetRenderDrawColor(renderer,255,225,105,Uint8(180+(ticks/250%2)*75));
        SDL_RenderRect(renderer,&player);
    }
    SDL_SetRenderClipRect(renderer,clipped ? &previous : nullptr);
    SDL_SetRenderDrawColor(renderer,197,163,97,235);
    SDL_RenderRect(renderer,&box);
}
}
