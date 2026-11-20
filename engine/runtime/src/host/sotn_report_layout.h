#pragma once
#include <SDL3/SDL.h>
#include <algorithm>

namespace SotnReportLayout {
struct Layout { SDL_FRect preview{}, title{}, description{}, capture{}, save{}, back{}; };
inline Layout make(SDL_FRect panel, float top, float u) {
    Layout l;
    const bool two=panel.w>=panel.h;
    const float pad=24*u, gap=24*u, bottom=panel.y+panel.h-(two ? 28 : 64)*u;
    const float width=panel.w-2*pad;
    const float right=two ? panel.x+pad+width*.46f+gap : panel.x+pad;
    const float rw=two ? width*.54f-gap : width;
    const float fields_top=two ? top : top+(bottom-top)*.32f;
    l.preview={panel.x+pad,top,two ? width*.46f : width,
               two ? bottom-top-58*u : (bottom-top)*.28f};
    l.title={right,fields_top+20*u,rw,36*u};
    l.description={right,fields_top+84*u,rw,std::max(36*u,bottom-fields_top-150*u)};
    l.save={right,bottom-40*u,rw,36*u};
    l.capture={panel.x+pad,bottom-40*u,two ? l.preview.w*.64f : width*.44f,36*u};
    l.back={two ? l.capture.x+l.capture.w+8*u : panel.x+pad+width*.48f,
            two ? bottom-40*u : bottom+4*u,
            two ? l.preview.w-l.capture.w-8*u : width*.44f,36*u};
    if (!two) {
        l.capture.y=bottom+4*u;
        l.save.y=bottom-40*u;
    }
    return l;
}
}
