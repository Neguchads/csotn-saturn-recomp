#pragma once
#include <algorithm>
#include <cstddef>

// Painel amplo em paisagem; uma coluna em retrato. Testado fora do renderer.
namespace SotnDevLayout {
struct Rect { float x, y, w, h; };
struct Layout {
    Rect panel{};
    float u = 1, column = 0, row = 0, gap = 0;
    float title_y = 0, footer_y = 0;
    float column_x[2]{}, section_y[2]{}, row_y[2]{};
    bool two = true;
    Rect cell(int c, std::size_t index) const {
        return {column_x[c], row_y[c] + float(index) * (row + gap), column, row};
    }
};
inline Layout make(int width, int height, std::size_t left, std::size_t right, float density = 1) {
    Layout l;
    if (width <= 0 || height <= 0) return l;
    l.panel = {width * .02f, height * .025f, width * .96f, height * .95f};
    l.two = width >= 640 && width >= height * .85f;
    const float count = float(l.two ? std::max(left, right) : left + right);
    const float needed = 24*2 + 92 + (l.two ? 28 : 56) + count*50 + 138;
    l.u = std::min({std::max(.25f, density * 1.35f), l.panel.h / needed,
                   l.panel.w / (l.two ? 1100.f : 520.f)});
    const float pad = 24*l.u, between = 24*l.u;
    l.column = (l.panel.w - 2*pad - (l.two ? between : 0)) / (l.two ? 2 : 1);
    l.row = 44*l.u;
    l.gap = 6*l.u;
    l.title_y = l.panel.y + pad + 40*l.u;
    l.footer_y = l.panel.y + l.panel.h - 124*l.u;
    for (int c = 0; c < 2; ++c) {
        l.column_x[c] = l.panel.x + pad + (l.two ? c*(l.column + between) : 0);
        l.section_y[c] = l.panel.y + pad + 92*l.u + 14*l.u +
            (!l.two && c == 1 ? 28*l.u + float(left)*50*l.u : 0);
        l.row_y[c] = l.section_y[c] + 18*l.u;
    }
    return l;
}
}
