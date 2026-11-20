// saturn-recomp runtime — VDP2: the picture of a field, composed in software
// from VRAM, colour RAM, the registers and VDP1's framebuffer.
//
// What is done: the four normal scroll screens (NBG0-NBG3) in cell mode
// (1x1 or 2x2 cells, 1- or 2-word pattern names with the supplement, plane
// sizes, map offsets, 16/256/2048 colours and RGB) and NBG0/NBG1 in bitmap
// mode; NBG0/NBG1's fractional scroll and coordinate increments (zoom);
// the sprite layer from VDP1's framebuffer, including rotation parameter A, every sprite type, palette and
// RGB mixed (SPCLMD), with its priorities and colour-calculation ratios;
// priorities with the chip's order for ties (sprite, NBG0, NBG1, NBG2,
// NBG3); special priority and special colour calculation by screen or by
// character, and colour calculation by the colour's MSB; colour calculation
// of the top two layers (the top one's ratio or the second's, or add), with
// the line colour screen (one colour, or one a line) as the second image
// under a top screen that LNCLEN names, or gradation (the screen BOKN names,
// blurred along the line, as the second image); windows 0 and 1, as
// rectangles or line windows, on the scroll screens, the sprite layer and
// colour calculation; the colour offsets A and B; the back screen (one
// colour or one a line); the display bit; the high resolutions (640 and 704
// dots), with double-density interlace composed as one picture of both
// fields' lines (448, 480 or 512), the scroll screens a dot row a line;
// shadows (normal, MSB and transparent) on the scroll screens, the back
// screen and the sprite itself.
// Extensões locais: scroll/zoom por linha, scroll por célula com ciclos de acesso,
// mosaico, janela de sprite, funções especiais por ponto, mistura estendida,
// RBG0/RBG1 A/B com matriz/mapas/coeficientes/screen-over e dimensões exclusivas.
// A composição ainda parte de um snapshot por campo. Não reconstrói alterações
// de registradores/VRAM durante a varredura nem a temporização de monitores
// exclusivos. Os testes sintéticos validam os contratos cobertos, não todo o jogo.
#include "saturn.h"
#include "state.h"
#include "video.h"
#include "vdp2_gpu_snapshot.h"
#include <algorithm>
#include <cstring>
#include <vector>

uint8_t g_vdp2_vram[0x80000], g_vdp2_cram[0x1000], g_vdp2_regs[0x200];

static uint16_t reg(uint32_t off) { return (uint16_t)(g_vdp2_regs[off] << 8 | g_vdp2_regs[off + 1]); }
static uint8_t vb(uint32_t a) { return g_vdp2_vram[a & 0x7FFFF]; }
static uint16_t vw(uint32_t a) { a &= 0x7FFFE; return (uint16_t)(g_vdp2_vram[a] << 8 | g_vdp2_vram[a + 1]); }
static uint32_t vl(uint32_t a) { return (uint32_t)vw(a) << 16 | vw(a + 2); }

static int32_t signed_bits(uint32_t value, int bits) {
    uint32_t sign = 1u << (bits - 1);
    return (int32_t)((value & (sign * 2 - 1)) ^ sign) - (int32_t)sign;
}

static uint16_t sprite_dot(int x, int y, bool dd) {
    uint16_t tvmr = vdp1_reg_read(0, 2);
    // O VDP2 duplica os pontos de 16 bits em 640/704 e pula pontos
    // do framebuffer de 8 bits em 320/352. Aplicar antes da rotação.
    int hreso = reg(0) & 6;
    if (!(tvmr & 4)) {
        if (!(tvmr & 1) && (hreso & 2)) x >>= 1;
        else if ((tvmr & 1) && !(hreso & 2)) x <<= 1;
    }
    if (!(tvmr & 2)) return vdp1_dot(x, y, dd);
    uint32_t table = (((uint32_t)(reg(0xBC) & 7) << 16 | reg(0xBE)) & ~0x41u) * 2;
    auto parameter = [&](uint32_t off, int bits, int shift) {
        return signed_bits(((uint32_t)vw(table + off) << 16 | vw(table + off + 2)) >> shift, bits);
    };
    int32_t sx = parameter(0, 23, 6), sy = parameter(4, 23, 6);
    int32_t vx = parameter(12, 13, 6), vy = parameter(16, 13, 6);
    int32_t dx = parameter(20, 12, 7), dy = parameter(24, 12, 7);
    if (!(reg(0xB2) & 1)) sx += y * vx;
    if (!(reg(0xB2) & 2)) sy += y * vy;
    sx = signed_bits((uint32_t)(sx >> 1), 20);
    sy = signed_bits((uint32_t)(sy >> 1), 20);
    return vdp1_dot((sx + x * dx) >> 9, (sy + x * dy) >> 9, false);
}

int vdp2_lines() {
    if (reg(0x00) & 4) return 480;
    switch (reg(0x00) >> 4 & 3) {
    case 1: return 240;
    case 2: return 256;
    default: return 224;
    }
}

static int cram_mode() { return reg(0x0E) >> 12 & 3; }

// A colour RAM entry as 0x00RRGGBB, and its MSB (the colour calculation bit).
static uint32_t cram(uint32_t i, bool* msb) {
    if (cram_mode() == 2) {
        uint32_t a = (i & 0x3FF) * 4;
        uint32_t v = (uint32_t)g_vdp2_cram[a] << 24 | g_vdp2_cram[a + 1] << 16 | g_vdp2_cram[a + 2] << 8 | g_vdp2_cram[a + 3];
        if (msb) *msb = v >> 31;
        return (v & 0xFF) << 16 | (v & 0xFF00) | (v >> 16 & 0xFF);
    }
    uint32_t a = (i & (cram_mode() == 1 ? 0x7FF : 0x3FF)) * 2;
    uint16_t v = (uint16_t)(g_vdp2_cram[a] << 8 | g_vdp2_cram[a + 1]);
    if (msb) *msb = v >> 15;
    return (uint32_t)((v & 0x1F) << 3) << 16 | ((v >> 5 & 0x1F) << 3) << 8 | ((v >> 10 & 0x1F) << 3);
}

static uint32_t rgb555(uint16_t v) {
    return (uint32_t)((v & 0x1F) << 3) << 16 | ((v >> 5 & 0x1F) << 3) << 8 | ((v >> 10 & 0x1F) << 3);
}

// One layer's pixel on a line.
struct Pix {
    uint32_t rgb;
    uint8_t prio;                               // 0: nothing here
    uint8_t ratio;                              // colour-calculation ratio, 0-31
    bool cc;                                    // colour calculation on
    bool offset;                                // colour offset on
    bool offset_b;                              // ... offset B rather than A
    bool spr, scc;                              // the character's special priority and colour calculation bits
    bool msb;                                   // the colour data's MSB
    bool shade;                                 // an MSB shadow: the sprite dot at half brightness
    uint8_t code;                               // código especial dos bits 3:1 da cor original
    bool direct;                                // RGB direto não tem prioridade especial por ponto
    bool coefficient_line;
    uint32_t line_rgb;
};

enum { L_SPRITE, L_RBG0, L_NBG0, L_NBG1, L_NBG2, L_NBG3, L_COUNT };

// ---- the scroll screens ---------------------------------------------------------------------
struct Nbg {
    bool on, opaque, bitmap, cell2x2, word1, cnsm;
    int colors;                                 // 0 16, 1 256, 2 2048, 3 32K RGB, 4 16M RGB
    uint16_t supp;                              // the pattern name supplement
    int plane_w, plane_h;                       // in pages
    uint32_t plane[16];                         // quatro planos NBG; dezesseis RBG
    unsigned map_side;
    uint32_t page_bytes;
    int bm_w, bm_h;                             // bitmap size
    uint32_t bm_addr;
    uint16_t bm_pal;
    bool bm_spr, bm_scc;
    uint32_t caos;                              // colour RAM offset, in entries
    int32_t sx, sy, dx, dy;                     // scroll and increments, 8 fraction bits
    uint8_t prio, ratio;
    bool cc, offset, offset_b;
    uint8_t line_flags, line_shift, mosaic_w, mosaic_h;
    uint32_t line_table;
    // Só vive neste snapshot; cada compose cria novas descrições de camada.
    mutable uint32_t cached_pnd = 0xFFFFFFFFu, cached_chr = 0, cached_pal = 0;
    mutable bool cached_hf = false, cached_vf = false, cached_spr = false, cached_scc = false;
};

static void setup_nbg(int n, Nbg& s) {
    s = Nbg{};
    s.map_side = 2;
    uint16_t bgon = reg(0x20);
    s.on = (bgon >> n & 1) || (n == 0 && (bgon & 0x20));
    s.opaque = bgon >> (8 + n) & 1;
    if (!s.on) return;
    uint16_t chctl = n < 2 ? reg(0x28) : reg(0x2A);
    int sh = (n & 1) * 8;
    if (n < 2) {
        uint16_t c = (uint16_t)(chctl >> sh);
        s.colors = n == 0 ? c >> 4 & 7 : c >> 4 & 3;
        s.bitmap = c >> 1 & 1;
        s.cell2x2 = c & 1;
        int bmsz = c >> 2 & 3;
        s.bm_w = bmsz & 2 ? 1024 : 512;
        s.bm_h = bmsz & 1 ? 512 : 256;
    } else {
        uint16_t c = (uint16_t)(chctl >> ((n - 2) * 4));
        s.colors = c >> 1 & 1;
        s.cell2x2 = c & 1;
    }
    uint16_t pncn = reg(0x30 + n * 2);
    s.word1 = pncn >> 15 & 1;
    s.cnsm = pncn >> 14 & 1;
    s.supp = pncn & 0x3FF;
    int plsz = reg(0x3A) >> (n * 2) & 3;
    s.plane_w = plsz == 0 ? 1 : 2;
    s.plane_h = plsz == 3 ? 2 : 1;
    uint32_t mpof = reg(0x3C) >> (n * 4) & 7;
    s.page_bytes = (s.cell2x2 ? 0x800u : 0x2000u) * (s.word1 ? 1 : 2);
    uint32_t mask = (uint32_t)(s.plane_w * s.plane_h - 1);
    for (int i = 0; i < 4; ++i) {
        uint16_t mp = reg(0x40 + n * 4 + (i / 2) * 2);
        uint32_t m = (mpof << 6 | (uint32_t)(mp >> ((i & 1) * 8) & 0x3F)) & ~mask;
        s.plane[i] = m * s.page_bytes;
    }
    if (s.bitmap) {
        s.bm_addr = mpof * 0x20000;
        uint16_t bmpn = (uint16_t)(reg(0x2C) >> sh);
        s.bm_pal = (uint16_t)((bmpn & 7) << 4);
        s.bm_spr = bmpn >> 5 & 1;
        s.bm_scc = bmpn >> 4 & 1;
    }
    s.colors = std::min(s.colors, 4);
    int caos_sh = n * 4;
    s.caos = (uint32_t)(reg(0xE4) >> caos_sh & 7) << 8;
    if (n < 2) {
        uint32_t b = 0x70 + n * 0x10;
        s.sx = (int32_t)((reg(b) & 0x7FF) << 8 | reg(b + 2) >> 8);
        s.sy = (int32_t)((reg(b + 4) & 0x7FF) << 8 | reg(b + 6) >> 8);
        s.dx = (int32_t)((reg(b + 8) & 7) << 8 | reg(b + 10) >> 8);
        s.dy = (int32_t)((reg(b + 12) & 7) << 8 | reg(b + 14) >> 8);
    } else {
        uint32_t b = 0x90 + (n - 2) * 4;
        s.sx = (int32_t)(reg(b) & 0x7FF) << 8;
        s.sy = (int32_t)(reg(b + 2) & 0x7FF) << 8;
        s.dx = s.dy = 0x100;
    }
    uint16_t prin = reg(0xF8 + (n / 2) * 2);
    s.prio = (uint8_t)(prin >> ((n & 1) * 8) & 7);
    uint16_t ccrn = reg(0x108 + (n / 2) * 2);
    s.ratio = (uint8_t)(ccrn >> ((n & 1) * 8) & 0x1F);
    s.cc = reg(0xEC) >> n & 1;
    s.offset = reg(0x110) >> n & 1;
    s.offset_b = reg(0x112) >> n & 1;
    uint16_t mosaic = reg(0x22);
    s.mosaic_w = mosaic >> n & 1 ? uint8_t((mosaic >> 8 & 15) + 1) : 1;
    s.mosaic_h = mosaic >> n & 1 ? uint8_t((mosaic >> 12 & 15) + 1) : 1;
    if (n < 2) {
        uint8_t control = (uint8_t)(reg(0x9A) >> (n * 8));
        s.line_flags = control & 14;
        s.line_shift = control >> 4 & 3;
        s.line_table = (uint32_t)(reg(0xA0 + n * 4) & 7) << 17 |
                       (uint32_t)(reg(0xA2 + n * 4) & 0xFFFE) << 1;
    }
}

// the dot of a character (or bitmap) at (x, y) in its own pixels: false if transparent
static bool dot(const Nbg& s, uint32_t base, int x, int y, int width, uint32_t pal, Pix& p) {
    uint32_t i = (uint32_t)(y * width + x);
    uint32_t v;
    switch (s.colors) {
    case 0: v = vb(base + i / 2); v = (i & 1) ? (v & 0xF) : (v >> 4); break;
    case 1: v = vb(base + i); break;
    case 2: v = vw(base + i * 2) & 0x7FF; break;
    case 3: {
        uint16_t w = vw(base + i * 2);
        if (!(w & 0x8000) && !s.opaque) return false;
        p.rgb = rgb555(w);
        p.msb = w >> 15;
        p.direct = true; p.code = 7;
        return true;
    }
    default: {
        uint32_t w = (uint32_t)vw(base + i * 4) << 16 | vw(base + i * 4 + 2);
        if (!(w & 0x80000000u) && !s.opaque) return false;
        p.rgb = (w & 0xFF) << 16 | (w & 0xFF00) | (w >> 16 & 0xFF);
        p.msb = w >> 31;
        p.direct = true; p.code = 7;
        return true;
    }
    }
    if (v == 0 && !s.opaque) return false;
    p.code = (uint8_t)(v >> 1 & 7);
    uint32_t index = s.colors == 0 ? pal * 16 + v : s.colors == 1 ? (pal & 0x70) * 16 + v : v;
    p.rgb = cram(index + s.caos, &p.msb);
    return true;
}

static bool source_pixel(const Nbg& s, uint32_t sx, uint32_t sy, Pix& p, int over_pattern = -1) {
    if (s.bitmap) {
        sx &= (uint32_t)s.bm_w - 1;
        sy &= (uint32_t)s.bm_h - 1;
        p.spr = s.bm_spr;
        p.scc = s.bm_scc;
        return dot(s, s.bm_addr, (int)sx, (int)sy, s.bm_w, s.bm_pal, p);
    }
    uint32_t pw = (uint32_t)s.plane_w * 512, ph = (uint32_t)s.plane_h * 512;
    sx &= pw * s.map_side - 1;
    sy &= ph * s.map_side - 1;
    unsigned pl = (sy >> (9 + (s.plane_h == 2))) * s.map_side + (sx >> (9 + (s.plane_w == 2)));
    uint32_t ix = sx & (pw - 1), iy = sy & (ph - 1);
    uint32_t page = (iy >> 9) * (uint32_t)s.plane_w + (ix >> 9);
    int cs = s.cell2x2 ? 16 : 8;
    unsigned cell_shift = s.cell2x2 ? 4 : 3;
    uint32_t cx = (ix & 511) >> cell_shift, cy = (iy & 511) >> cell_shift;
    uint32_t cpr = 512u >> cell_shift;
    uint32_t pnd_addr = s.plane[pl] + page * s.page_bytes + (cy * cpr + cx) * (s.word1 ? 2 : 4);
    uint32_t chr, pal;
    bool hf, vf;
    if (over_pattern < 0 && s.cached_pnd == pnd_addr) {
        chr = s.cached_chr; pal = s.cached_pal;
        hf = s.cached_hf; vf = s.cached_vf;
        p.spr = s.cached_spr; p.scc = s.cached_scc;
    } else {
        if (s.word1 || over_pattern >= 0) {
            uint16_t w = over_pattern >= 0 ? uint16_t(over_pattern) : vw(pnd_addr);
            p.spr = s.supp >> 9 & 1;
            p.scc = s.supp >> 8 & 1;
            pal = s.colors == 0 ? (uint32_t)(w >> 12 & 0xF) | (s.supp >> 1 & 0x70) : (uint32_t)(w >> 8 & 0x70);
            if (!s.cnsm) {
                hf = w & 0x400; vf = w & 0x800;
                chr = s.cell2x2 ? ((w & 0x3FFu) << 2) | (s.supp & 3u) | ((s.supp & 0x1Cu) << 10)
                                : (w & 0x3FFu) | ((s.supp & 0x1Fu) << 10);
            } else {
                hf = vf = false;
                chr = s.cell2x2 ? ((w & 0xFFFu) << 2) | (s.supp & 3u) | ((s.supp & 0x10u) << 10)
                                : (w & 0xFFFu) | ((s.supp & 0x1Cu) << 10);
            }
        } else {
            uint16_t w0 = vw(pnd_addr), w1 = vw(pnd_addr + 2);
            hf = w0 & 0x4000; vf = w0 & 0x8000;
            p.spr = w0 >> 13 & 1;
            p.scc = w0 >> 12 & 1;
            pal = w0 & 0x7F;
            chr = w1 & 0x7FFF;
        }
        if (over_pattern < 0) {
            s.cached_pnd = pnd_addr; s.cached_chr = chr; s.cached_pal = pal;
            s.cached_hf = hf; s.cached_vf = vf;
            s.cached_spr = p.spr; s.cached_scc = p.scc;
        }
    }
    int px = int(ix & (uint32_t(cs) - 1)), py = int(iy & (uint32_t(cs) - 1));
    if (hf) px = cs - 1 - px;
    if (vf) py = cs - 1 - py;
    static const uint32_t kCellBytes[] = {32, 64, 128, 128, 256};
    uint32_t base = chr * 0x20;
    if (s.cell2x2) base += (uint32_t)((py >> 3) * 2 + (px >> 3)) * kCellBytes[s.colors];
    return dot(s, base, px & 7, py & 7, 8, pal, p);
}

static void nbg_coordinates(const Nbg& s, int x, int y, int32_t& fx, int32_t& fy) {
    if (s.mosaic_w > 1) x -= x % s.mosaic_w;
    if (s.mosaic_h > 1) y -= y % s.mosaic_h;
    int32_t origin_x = s.sx, origin_y = s.sy, dx = s.dx;
    int row = y;
    if (s.line_flags) {
        unsigned fields = unsigned(bool(s.line_flags & 2)) + unsigned(bool(s.line_flags & 4)) +
                          unsigned(bool(s.line_flags & 8));
        uint32_t address = s.line_table + (uint32_t(y) >> s.line_shift) * fields * 4;
        if (s.line_flags & 2) { origin_x += int32_t(vl(address) >> 8 & 0x7FFFF); address += 4; }
        if (s.line_flags & 4) {
            origin_y += int32_t(vl(address) >> 8 & 0x7FFFF);
            row &= (1 << s.line_shift) - 1;
            address += 4;
        }
        if (s.line_flags & 8) dx = int32_t(vl(address) >> 8 & 0x7FF);
    }
    fx = origin_x + x * dx;
    fy = origin_y + row * s.dy;
}

static bool nbg_pixel(const Nbg& s, int x, int y, Pix& p, uint32_t cell_scroll = 0) {
    int32_t fx, fy;
    nbg_coordinates(s, x, y, fx, fy);
    return source_pixel(s, uint32_t(fx) >> 8, (uint32_t(fy) + cell_scroll) >> 8, p);
}

static uint32_t g_vcell_last[2];
void vdp2_reset_fetch() { g_vcell_last[0] = g_vcell_last[1] = 0; }

struct CellScroll {
    uint32_t offset = 0;
    bool delay = false, repeat = false;
};

static unsigned cell_scroll_accesses(CellScroll (&access)[2]) {
    unsigned stride = 0;
    for (unsigned slot = 0; slot < 8; ++slot) {
        bool found[2] = {};
        for (unsigned bank = 0; bank < 4; ++bank) {
            unsigned code = reg(0x10 + bank * 4 + (slot / 4) * 2) >> ((3 - slot % 4) * 4) & 15;
            if (code == 12 || code == 13) found[code - 12] = true;
        }
        for (unsigned n = 0; n < 2; ++n) {
            if (!found[n] || !(reg(0x9A) >> (n * 8) & 1)) continue;
            access[n].offset = stride;
            access[n].delay = slot >= 3;
            access[n].repeat = n == 0 && slot >= 2;
            stride += 4;
        }
    }
    return stride;
}

static void cell_scroll_line(const Nbg& screen, unsigned n, const CellScroll& access,
                             unsigned stride, int y, std::vector<uint32_t>& output) {
    uint32_t address = (uint32_t(reg(0x9C) & 7) << 17 | uint32_t(reg(0x9E) & 0xFFFE) << 1) + access.offset;
    auto fetch = [&](bool first) {
        if (first && access.repeat && access.delay) return g_vcell_last[n];
        uint32_t previous = g_vcell_last[n];
        g_vcell_last[n] = vl(address) >> 8 & 0x7FFFF;
        if (!first || !access.repeat) address += stride;
        return access.delay ? previous : g_vcell_last[n];
    };
    int32_t fx, fy;
    nbg_coordinates(screen, 0, y, fx, fy);
    uint32_t cell = uint32_t(fx) >> 11, scroll = fetch(true);
    for (size_t x = 0; x < output.size(); ++x) {
        nbg_coordinates(screen, int(x), y, fx, fy);
        uint32_t next = uint32_t(fx) >> 11;
        if (screen.mosaic_w == 1 && screen.mosaic_h == 1 && next != cell) {
            cell = next;
            scroll = fetch(false);
        }
        output[x] = scroll;
    }
    // O fetch da célula seguinte alimenta o atraso na próxima linha.
    if (screen.mosaic_w == 1 && screen.mosaic_h == 1) {
        nbg_coordinates(screen, int(output.size()), y, fx, fy);
        if ((uint32_t(fx) >> 11) != cell) fetch(false);
    }
}

// ---- rotação: aritmética fixa da tabela A/B, sem alterar a imagem original ----
struct RotationParam {
    int32_t start[3], row[2], step[2], matrix[6], view[3], center[3], move[2], scale[2];
    int64_t ka;
    int32_t ka_row, ka_step;
    uint8_t control, line_colour;
    int64_t coefficient;
    bool transparent = true;
};

static RotationParam rotation_parameter(unsigned index) {
    uint32_t address = (((uint32_t)(reg(0xBC) & 7) << 17 | (uint32_t)reg(0xBE) << 1) & 0xFFF7C) + index * 0x80;
    RotationParam p{};
    auto fixed = [&](uint32_t offset, int bits) { return signed_bits(vl(address + offset) >> 6, bits); };
    for (unsigned n = 0; n < 3; ++n) {
        p.start[n] = fixed(n * 4, 23);
        p.view[n] = signed_bits(vw(address + 0x34 + n * 2), 14);
        p.center[n] = signed_bits(vw(address + 0x3C + n * 2), 14);
    }
    for (unsigned n = 0; n < 2; ++n) {
        p.row[n] = fixed(0x0C + n * 4, 13);
        p.step[n] = fixed(0x14 + n * 4, 13);
        p.move[n] = fixed(0x44 + n * 4, 24);
        p.scale[n] = signed_bits(vl(address + 0x4C + n * 4), 24);
    }
    for (unsigned n = 0; n < 6; ++n) p.matrix[n] = fixed(0x1C + n * 4, 14);
    p.ka = (int64_t)(vl(address + 0x54) >> 6) + int64_t(reg(0xB6) >> (index * 8) & 7) * (1ll << 26);
    p.ka_row = fixed(0x58, 20);
    p.ka_step = fixed(0x5C, 20);
    p.control = uint8_t(reg(0xB4) >> (index * 8));
    return p;
}

static Nbg rotation_screen(unsigned index, bool rbg1, const Nbg& normal) {
    Nbg s = rbg1 ? normal : Nbg{};
    s.on = rbg1 ? (reg(0x20) & 0x20) != 0 : (reg(0x20) & 0x10) != 0;
    s.map_side = 4;
    if (!rbg1) {
        uint16_t ctl = reg(0x2A), pn = reg(0x38), bitmap = reg(0x2E);
        s.opaque = reg(0x20) & 0x1000;
        s.colors = std::min(int(ctl >> 12 & 7), 4);
        s.bitmap = ctl & 0x200;
        s.cell2x2 = ctl & 0x100;
        s.word1 = pn & 0x8000; s.cnsm = pn & 0x4000; s.supp = pn & 0x3FF;
        s.bm_w = 512; s.bm_h = ctl & 0x400 ? 512 : 256;
        s.bm_pal = uint16_t((bitmap & 7) << 4);
        s.bm_spr = bitmap & 0x20; s.bm_scc = bitmap & 0x10;
        s.caos = uint32_t(reg(0xE6) & 7) << 8;
        s.prio = uint8_t(reg(0xFC) & 7); s.ratio = uint8_t(reg(0x10C) & 31);
        s.cc = reg(0xEC) & 0x10; s.offset = reg(0x110) & 0x10; s.offset_b = reg(0x112) & 0x10;
        s.mosaic_w = reg(0x22) & 0x10 ? uint8_t((reg(0x22) >> 8 & 15) + 1) : 1;
        s.mosaic_h = reg(0x22) & 0x10 ? uint8_t((reg(0x22) >> 12 & 15) + 1) : 1;
    }
    unsigned size = reg(0x3A) >> (8 + index * 4) & 3;
    s.plane_w = 1 << (size & 1); s.plane_h = 1 << (size >> 1);
    s.page_bytes = (s.cell2x2 ? 0x800u : 0x2000u) * (s.word1 ? 1 : 2);
    unsigned map_offset = reg(0x3E) >> (index * 4) & 7;
    s.bm_addr = map_offset * 0x20000;
    for (unsigned n = 0; n < 16; ++n) {
        unsigned map = (map_offset << 6) | (reg(0x50 + index * 0x10 + (n / 2) * 2) >> ((n & 1) * 8) & 0x3F);
        map &= ~unsigned(s.plane_w * s.plane_h - 1);
        s.plane[n] = map * s.page_bytes;
    }
    return s;
}

static void rotation_coefficient(RotationParam& p, int x, int y) {
    if (!(p.control & 1)) return;
    uint16_t ram = reg(0x0E);
    bool per_dot = ram & 0x8000;
    for (unsigned bank = 0; bank < 4; ++bank) {
        unsigned physical = bank;
        if (!(ram >> (8 + bank / 2) & 1)) physical &= ~1u;
        per_dot |= (ram >> (physical * 2) & 3) == 1;
    }
    if (!per_dot && x != 0) return;
    uint32_t entry = uint32_t((p.ka + int64_t(y) * p.ka_row + int64_t(x) * p.ka_step) >> 10);
    bool small = p.control & 2;
    uint32_t address = entry * (small ? 2 : 4), raw;
    if (ram & 0x8000) {
        address = (address | 0x800) & 0xFFF;
        auto byte = [&](uint32_t at) { return uint32_t(g_vdp2_cram[at & 0xFFF]); };
        raw = small ? byte(address) << 8 | byte(address + 1)
                    : byte(address) << 24 | byte(address + 1) << 16 | byte(address + 2) << 8 | byte(address + 3);
    } else {
        unsigned bank = address >> 17 & 3;
        if (!(ram >> (8 + bank / 2) & 1)) bank &= ~1u;
        // Sem acesso de coeficiente nesse banco, conservar o último valor lido.
        if (per_dot && (ram >> (bank * 2) & 3) != 1) return;
        raw = small ? vw(address) : vl(address);
    }
    unsigned mode = p.control >> 2 & 3;
    p.transparent = raw >> (small ? 15 : 31);
    p.line_colour = small ? 0 : uint8_t(raw >> 24 & 0x7F);
    p.coefficient = signed_bits(raw, small ? 15 : 24);
    p.coefficient *= small ? (mode == 3 ? 16384 : 64) : (mode == 3 ? 256 : 1);
}

static bool rotation_pixel(const Nbg& s, const RotationParam& p, unsigned parameter,
                           int x, int y, Pix& pixel) {
    if ((p.control & 1) && p.transparent) return false;
    int64_t start[3] = {p.start[0] + int64_t(y) * p.row[0], p.start[1] + int64_t(y) * p.row[1], p.start[2]};
    int64_t source[2];
    for (unsigned axis = 0; axis < 2; ++axis) {
        int64_t projected = 0, translated = int64_t(p.center[axis]) * 1024 + p.move[axis];
        for (unsigned n = 0; n < 3; ++n) {
            int32_t m = p.matrix[axis * 3 + n];
            projected += int64_t(m) * (start[n] - int64_t(p.view[n]) * 1024);
            translated += int64_t(m) * (p.view[n] - p.center[n]);
        }
        projected >>= 10;
        projected += int64_t(x) * ((int64_t(p.matrix[axis * 3]) * p.step[0] +
                                  int64_t(p.matrix[axis * 3 + 1]) * p.step[1]) >> 10);
        int64_t scale = p.scale[axis];
        if (p.control & 1) {
            unsigned mode = p.control >> 2 & 3;
            if (mode == 0 || mode == axis + 1) scale = p.coefficient;
            if (mode == 3 && axis == 0) translated = p.coefficient * 4;
        }
        source[axis] = (((scale * projected) >> 16) + translated) >> 10;
    }
    unsigned over = reg(0x3A) >> (10 + parameter * 4) & 3;
    int64_t width = s.bitmap ? s.bm_w : s.plane_w * 2048;
    int64_t height = s.bitmap ? s.bm_h : s.plane_h * 2048;
    if (over == 3) width = height = 512;
    bool outside = source[0] < 0 || source[1] < 0 || source[0] >= width || source[1] >= height;
    if (over && outside) {
        if (over != 1 || s.bitmap) return false;
        return source_pixel(s, uint32_t(source[0]) & 7, uint32_t(source[1]) & 7, pixel,
                            reg(0xB8 + parameter * 2));
    }
    return source_pixel(s, uint32_t(source[0]), uint32_t(source[1]), pixel);
}

// ---- the sprite layer -----------------------------------------------------------------------
struct SpriteType { int pr_shift, pr_mask, cc_shift, cc_mask, dc_mask; };
static const SpriteType kTypes[16] = {
    {14, 3, 11, 7, 0x7FF}, {13, 7, 11, 3, 0x7FF}, {14, 1, 11, 7, 0x7FF}, {13, 3, 11, 3, 0x7FF},
    {13, 3, 10, 7, 0x3FF}, {12, 7, 11, 1, 0x7FF}, {12, 7, 10, 3, 0x3FF}, {12, 7, 9, 7, 0x1FF},
    {7, 1, 0, 0, 0x7F},    {7, 1, 6, 1, 0x3F},    {6, 3, 0, 0, 0x3F},    {0, 0, 6, 3, 0x3F},
    {7, 1, 0, 0, 0xFF},    {7, 1, 6, 1, 0xFF},    {6, 3, 0, 0, 0xFF},    {0, 0, 6, 3, 0xFF},
};

// S_SHADOW is a dot that shows nothing itself and halves the brightness of what is under it.
enum SpriteDot { S_NONE, S_DOT, S_SHADOW };

static SpriteDot sprite_pixel(uint16_t d, Pix& p) {
    uint16_t spctl = reg(0xE0);
    int type = spctl & 0xF;
    bool window = spctl >> 4 & 1;
    bool mixed = spctl >> 5 & 1;
    int ccmode = spctl >> 12 & 3, ccnum = spctl >> 8 & 7;
    if (d == 0) return S_NONE;
    auto prio_of = [](int i) { return (uint8_t)(reg(0xF0 + (i / 2) * 2) >> ((i & 1) * 8) & 7); };
    auto ratio_of = [](int i) { return (uint8_t)(reg(0x100 + (i / 2) * 2) >> ((i & 1) * 8) & 0x1F); };
    bool msb;
    SpriteDot kind = S_DOT;
    if (mixed && (d & 0x8000)) {
        p.rgb = rgb555(d);
        p.prio = prio_of(0);
        p.ratio = ratio_of(0);
        msb = true;
    } else {
        const SpriteType& t = kTypes[type];
        if (type >= 8) d &= 0xFF;
        uint16_t dc = d & t.dc_mask;
        // Types 2-7 leave the MSB free, for the sprite window when SPWINEN is set and for shadows when not.
        bool msb_shadow = type >= 2 && type <= 7 && !window && (d & 0x8000);
        if (dc == t.dc_mask - 1) kind = S_SHADOW;                       // a normal shadow
        else if (msb_shadow && !(d & 0x7FFF)) {
            if (!(reg(0xE2) >> 8 & 1)) return S_NONE;                    // transparent shadows off (TPSDSL)
            kind = S_SHADOW;
        } else if (dc == 0) return S_NONE;
        p.shade = msb_shadow;
        int pr = t.pr_mask ? d >> t.pr_shift & t.pr_mask : 0;
        int cc = t.cc_mask ? d >> t.cc_shift & t.cc_mask : 0;
        p.prio = prio_of(pr);
        p.ratio = ratio_of(cc);
        uint32_t caos = (uint32_t)(reg(0xE6) >> 4 & 7) << 8;
        p.rgb = cram(dc + caos, &msb);
    }
    switch (ccmode) {
    case 0: p.cc = p.prio <= ccnum; break;
    case 1: p.cc = p.prio == ccnum; break;
    case 2: p.cc = p.prio >= ccnum; break;
    default: p.cc = msb; break;
    }
    p.cc = p.cc && (reg(0xEC) >> 6 & 1);
    p.offset = reg(0x110) >> 6 & 1;
    p.offset_b = reg(0x112) >> 6 & 1;
    return p.prio ? kind : S_NONE;
}

// ---- windows ----------------------------------------------------------------------------------
// Window 0 or 1 on one line: the line in its Y range, and its X span there.
struct Win { bool on_line; int xs, xe; };

static Win window_line(int w, int y, bool dd) {
    uint32_t b = 0xC0 + w * 8;
    int xs = reg(b) & 0x3FF, xe = reg(b + 4) & 0x3FF;
    uint16_t lwta = reg(0xD8 + w * 4);
    if (lwta & 0x8000) {                         // a line window: a span a line from the table
        uint32_t a = (((uint32_t)(lwta & 7) << 16 | reg(0xDA + w * 4)) & 0x7FFFE) * 2 + (uint32_t)y * 4;
        xs = vw(a) & 0x3FF;
        xe = vw(a + 2) & 0x3FF;
    }
    if (!(reg(0x00) & 2)) { xs >>= 1; xe >>= 1; } // X counts half dots below the high resolutions
    int ys = reg(b + 2) & 0x3FF, ye = reg(b + 6) & 0x3FF;
    if (!dd) { ys &= 0x1FF; ye &= 0x1FF; }
    return {y >= ys && y <= ye, xs, xe};
}

// Whether a layer's window control byte (WCTLA-D) makes dot x transparent: each enabled window's
// area (bit 0 or 2: inside when clear, outside when set), joined by OR or by AND (bit 7).
static bool windowed(uint8_t ctl, const Win w[2], int x, bool sprite_window = false) {
    bool any = false, all = true, used = false;
    for (int i = 0; i < 2; ++i) {
        if (!(ctl >> (i * 2 + 1) & 1)) continue;
        bool in = w[i].on_line && x >= w[i].xs && x <= w[i].xe;
        bool area = (ctl >> (i * 2) & 1) ? !in : in;
        any |= area;
        all &= area;
        used = true;
    }
    if (ctl & 0x20) {
        bool area = sprite_window != bool(ctl & 0x10);
        any |= area;
        all &= area;
        used = true;
    }
    return used && (ctl & 0x80 ? all : any);
}

// ---- composing ------------------------------------------------------------------------------
static void note_snapshot_limits() {
    static bool rotation_noted = false, exclusive_noted = false;
    if ((reg(0x20) & 0x30) && !rotation_noted) {
        rotation_noted = true;
        sat_note("VDP2: rotation uses field snapshots; mid-frame parameter reloads are not reconstructed");
    }
    if ((reg(0) & 4) && !exclusive_noted) {
        exclusive_noted = true;
        sat_note("VDP2: exclusive monitor dimensions are composed; raster timing remains NTSC");
    }
}

static uint32_t apply_offset(uint32_t rgb, bool b) {
    uint32_t base = b ? 0x11A : 0x114;
    int out = 0;
    for (int i = 0; i < 3; ++i) {
        int o = (int)((uint32_t)(reg(base + i * 2) & 0x1FF) << 23) >> 23;
        int c = (int)(rgb >> (16 - i * 8) & 0xFF) + o;
        out = out << 8 | std::clamp(c, 0, 255);
    }
    return (uint32_t)out;
}

bool vdp2_gpu_snapshot(Vdp2GpuSnapshot& out, const char** fallback_reason) {
    // Modos sequenciais/rotação permanecem no compositor CPU completo.
    const uint16_t tvmd = reg(0);
    const char* reason = !(tvmd & 0x8000) ? "display off"
                       : tvmd & 4 ? "exclusive monitor mode"
                       : reg(0x20) & 0x30 ? "rotation screen"
                       : reg(0x9A) & 0x0101 ? "vertical cell scroll"
                       : reg(0xEC) & 0x8000 ? "gradation" : nullptr;
    if (fallback_reason) *fallback_reason = reason;
    if (reason) return false;
    static const unsigned widths[] = {320, 352, 640, 704};
    const unsigned width = widths[tvmd & 3];
    const bool dd = (tvmd >> 6 & 3) == 3;
    const unsigned height = unsigned(vdp2_lines()) * (dd ? 2 : 1);
    out.params = {};
    out.params[0] = width; out.params[1] = height; out.params[2] = dd;
    for (unsigned n = 0; n < 4; ++n) {
        Nbg s; setup_nbg(int(n), s);
        auto* p = out.params.data() + 4 + n * 32;
        p[0]=s.on; p[1]=s.opaque; p[2]=s.bitmap; p[3]=s.cell2x2;
        p[4]=s.word1; p[5]=s.cnsm; p[6]=unsigned(s.colors); p[7]=s.supp;
        p[8]=unsigned(s.plane_w); p[9]=unsigned(s.plane_h); p[10]=s.page_bytes;
        p[11]=unsigned(s.bm_w); p[12]=unsigned(s.bm_h); p[13]=s.bm_addr;
        p[14]=s.bm_pal; p[15]=s.bm_spr; p[16]=s.bm_scc; p[17]=s.caos;
        p[18]=uint32_t(s.sx); p[19]=uint32_t(s.sy); p[20]=uint32_t(s.dx); p[21]=uint32_t(s.dy);
        p[22]=s.prio; p[23]=s.ratio; p[24]=s.cc; p[25]=s.offset; p[26]=s.offset_b;
        for (unsigned i=0; i<4; ++i) p[27+i]=s.plane[i];
        if (s.on) p[31] = uint32_t(s.mosaic_w - 1) | uint32_t(s.mosaic_h - 1) << 4 |
                         uint32_t(s.line_flags) << 8 | uint32_t(s.line_shift) << 12;
    }
    out.bytes.resize(0x81200 + size_t(width) * height * 4);
    std::memcpy(out.bytes.data(), g_vdp2_vram, sizeof g_vdp2_vram);
    std::memcpy(out.bytes.data()+0x80000, g_vdp2_cram, sizeof g_vdp2_cram);
    std::memcpy(out.bytes.data()+0x81000, g_vdp2_regs, sizeof g_vdp2_regs);
    for (unsigned y=0; y<height; ++y) for (unsigned x=0; x<width; ++x) {
        const uint32_t value = sprite_dot(int(x), int(y), dd);
        std::memcpy(out.bytes.data()+0x81200+(size_t(y)*width+x)*4, &value, 4);
    }
    return true;
}

void vdp2_compose(Frame& f) {
    uint16_t tvmd = reg(0x00);
    static const int kWidths[4] = {320, 352, 640, 704};
    int hreso = tvmd & 7;
    f.w = kWidths[hreso & 3];
    bool dd = hreso < 4 && (tvmd >> 6 & 3) == 3; // exclusivos já têm 480 linhas
    f.h = vdp2_lines() * (dd ? 2 : 1);
    f.px.assign((size_t)f.w * f.h, 0);
    if (!(tvmd & 0x8000)) return;                // display off
    note_snapshot_limits();
    Nbg nbg[4];
    for (int n = 0; n < 4; ++n) setup_nbg(n, nbg[n]);
    bool rbg0_on = reg(0x20) & 0x10, rbg1_on = reg(0x20) & 0x20;
    RotationParam rotation[2] = {rotation_parameter(0), rotation_parameter(1)};
    Nbg rbg[2] = {rotation_screen(0, false, nbg[0]), rotation_screen(1, false, nbg[0])};
    Nbg rbg1 = rotation_screen(1, true, nbg[0]);
    if (rbg0_on && rbg1_on) for (int n = 1; n < 4; ++n) nbg[n].on = false;
    CellScroll cell_access[2];
    unsigned cell_stride = cell_scroll_accesses(cell_access);
    std::vector<uint32_t> cells[2];
    for (unsigned n = 0; n < 2; ++n)
        if (nbg[n].on && !(n == 0 && rbg1_on) && (reg(0x9A) >> (n * 8) & 1)) cells[n].resize(f.w);
    std::vector<Pix> rotated[2];
    std::vector<Pix> rotation_mosaic_line[2];
    if (rbg0_on) rotated[0].resize(f.w);
    if (rbg1_on) rotated[1].resize(f.w);
    uint32_t bk = ((uint32_t)(reg(0xAC) & 7) << 16 | reg(0xAE)) * 2;
    bool bk_lines = reg(0xAC) & 0x8000;
    bool add = reg(0xEC) >> 8 & 1;
    bool second_ratio = reg(0xEC) >> 9 & 1;      // CCRTMD: the ratio is the second image's
    Pix back{};
    back.offset = reg(0x110) >> 5 & 1;
    back.offset_b = reg(0x112) >> 5 & 1;
    back.ratio = (uint8_t)(reg(0x10E) >> 8 & 0x1F);
    // the line colour screen: inserted as the second image under a top screen LNCLEN names
    uint16_t lnclen = reg(0xE8);
    uint32_t lc_table = ((uint32_t)(reg(0xA8) & 7) << 16 | reg(0xAA)) * 2;
    bool lc_lines = reg(0xA8) & 0x8000;
    Pix line_colour{};
    line_colour.ratio = (uint8_t)(reg(0x10E) & 0x1F);
    line_colour.cc = reg(0xEC) & 0x20;
    uint16_t sfprmd = reg(0xEA), sfccmd = reg(0xEE);
    uint8_t wctl[L_COUNT] = {(uint8_t)(reg(0xD4) >> 8), (uint8_t)reg(0xD4), (uint8_t)reg(0xD0), (uint8_t)(reg(0xD0) >> 8),
                             (uint8_t)reg(0xD2), (uint8_t)(reg(0xD2) >> 8)};
    uint8_t cc_wctl = (uint8_t)(reg(0xD6) >> 8);
    uint16_t sdctl = reg(0xE2);
    // gradation: the screen BOKN names, blurred along the line, as the second image wherever it is the
    // top or second image of a colour calculation
    static const int kGradLayer[8] = {L_SPRITE, L_RBG0, L_NBG0, -1, L_NBG1, L_NBG2, L_NBG3, -1};
    int grad = reg(0xEC) & 0x8000 && hreso < 2 && cram_mode() == 0 ? kGradLayer[reg(0xEC) >> 12 & 7] : -1;
    bool extended = (reg(0xEC) & 0x400) && hreso < 2 && grad < 0;
    auto average = [](uint32_t a, uint32_t b) { return (a + b - ((a ^ b) & 0x010101)) >> 1; };
    std::vector<uint32_t> blur(grad >= 0 ? f.w : 0);
    for (int y = 0; y < f.h; ++y) {
        int line = dd ? y >> 1 : y;
        Win win[2] = {window_line(0, y, dd), window_line(1, y, dd)};
        back.rgb = rgb555(vw(bk + (bk_lines ? (uint32_t)line * 2 : 0)));
        // a table word a line, a line of either field in double-density interlace
        line_colour.rgb = cram(vw(lc_table + (lc_lines ? (uint32_t)y * 2 : 0)) & 0x7FF, nullptr);
        for (unsigned n = 0; n < 2; ++n)
            if (!cells[n].empty()) cell_scroll_line(nbg[n], n, cell_access[n], cell_stride, y, cells[n]);
        if (rbg0_on || rbg1_on) {
            uint16_t line_index = vw(lc_table + (lc_lines ? (uint32_t)y * 2 : 0));
            int last_native_x = -1;
            bool coefficient_lines[2] = {};
            uint32_t coefficient_rgbs[2] = {};
            for (int x = 0; x < f.w; ++x) {
                int native_x = hreso & 2 ? x / 2 : x;
                if (native_x != last_native_x) {
                    if (native_x == 0) for (auto& parameter : rotation) {
                        parameter.transparent = true;
                        parameter.coefficient = 0;
                        parameter.line_colour = 0;
                    }
                    for (auto& parameter : rotation) rotation_coefficient(parameter, native_x, line);
                    last_native_x = native_x;
                }
                auto sample = [&](const Nbg& screen, unsigned parameter, Pix& p) {
                    p = {};
                    int mx = native_x - native_x % screen.mosaic_w;
                    int my = line - line % screen.mosaic_h;
                    if (!rotation_pixel(screen, rotation[parameter], parameter, mx, my, p)) return;
                    p.prio = 1;                // marca de pixel presente; prioridade aplicada na composição
                };
                auto line_for = [&](unsigned parameter, unsigned output) {
                    unsigned mode = reg(0xB0) & 3, selected = parameter;
                    bool combine = true;
                    if (mode == 0) { combine = parameter == 0; selected = 0; }
                    else if (mode == 1) { combine = parameter == 1; selected = rbg1_on ? 0 : 1; }
                    else if (mode == 2 || rbg1_on) selected = 0;
                    coefficient_lines[output] = combine && (rotation[selected].control & 0x11) == 0x11;
                    coefficient_rgbs[output] = cram((line_index & 0x780) | rotation[selected].line_colour, nullptr);
                };
                if (rbg0_on) {
                    unsigned parameter = 0;
                    switch (reg(0xB0) & 3) {
                    case 1: parameter = 1; break;
                    case 2: parameter = (rotation[0].control & 1) && rotation[0].transparent; break;
                    case 3: parameter = windowed(uint8_t(reg(0xD6) & 0x8F), win, x); break;
                    }
                    if (native_x % rbg[parameter].mosaic_w) {
                        int anchor = (native_x - native_x % rbg[parameter].mosaic_w) * (hreso & 2 ? 2 : 1);
                        rotated[0][x] = rotated[0][anchor];
                    } else sample(rbg[parameter], parameter, rotated[0][x]);
                    line_for(parameter, 0);
                    rotated[0][x].coefficient_line = coefficient_lines[0];
                    rotated[0][x].line_rgb = coefficient_rgbs[0];
                    if (native_x % rbg[parameter].mosaic_w) {
                        int anchor = (native_x - native_x % rbg[parameter].mosaic_w) * (hreso & 2 ? 2 : 1);
                        rotated[0][x].coefficient_line = rotated[0][anchor].coefficient_line;
                        rotated[0][x].line_rgb = rotated[0][anchor].line_rgb;
                    }
                }
                if (rbg1_on) {
                    if (native_x % rbg1.mosaic_w) {
                        int anchor = (native_x - native_x % rbg1.mosaic_w) * (hreso & 2 ? 2 : 1);
                        rotated[1][x] = rotated[1][anchor];
                    } else {
                        sample(rbg1, 1, rotated[1][x]);
                        line_for(1, 1);
                        rotated[1][x].coefficient_line = coefficient_lines[1];
                        rotated[1][x].line_rgb = coefficient_rgbs[1];
                    }
                }
            }
            for (unsigned output = 0; output < 2; ++output) {
                if (rotated[output].empty()) continue;
                unsigned height = output == 0 ? rbg[0].mosaic_h : rbg1.mosaic_h;
                if (line % height == 0) rotation_mosaic_line[output] = rotated[output];
                else rotated[output] = rotation_mosaic_line[output];
            }
        }
        if (grad >= 0) {
            // each dot averaged with the average of the two before it; a dot the screen does not draw is black
            auto avg = [](uint32_t a, uint32_t b) { return (a + b - ((a ^ b) & 0x010101)) >> 1; };
            uint32_t prev[2] = {0, 0};
            for (int x = 0; x < f.w; ++x) {
                Pix p{};
                bool drawn;
                if (grad == L_SPRITE) drawn = sprite_pixel(sprite_dot(x, y, dd), p) == S_DOT;
                else if (grad == L_RBG0) { p = rbg0_on ? rotated[0][x] : Pix{}; drawn = p.prio; }
                else if (grad == L_NBG0 && rbg1_on) { p = rotated[1][x]; drawn = p.prio; }
                else {
                    unsigned n = unsigned(grad - L_NBG0);
                    uint32_t vc = n < 2 && !cells[n].empty() ? cells[n][x] : 0;
                    drawn = nbg[n].on && nbg_pixel(nbg[n], x, y, p, vc);
                }
                uint32_t here = drawn ? p.rgb : 0;
                if (x == 0) prev[0] = prev[1] = here;
                blur[x] = avg(avg(prev[0], prev[1]), here);
                prev[0] = prev[1];
                prev[1] = here;
            }
        }
        for (int x = 0; x < f.w; ++x) {
            Pix l[L_COUNT];
            int top = -1, second = -1, third = -1;
            auto consider = [&](int i) {
                if (!l[i].prio) return;
                if (top < 0 || l[i].prio > l[top].prio) { third = second; second = top; top = i; }
                else if (second < 0 || l[i].prio > l[second].prio) { third = second; second = i; }
                else if (third < 0 || l[i].prio > l[third].prio) third = i;
            };
            // in the order that wins ties: sprite, then NBG0..NBG3
            l[L_SPRITE] = {};
            uint8_t shadow = 0;                  // the priority of a shadow dot here
            uint16_t sprite_data = sprite_dot(x, y, dd), spctl = reg(0xE0);
            int sprite_type = spctl & 15;
            bool sprite_window = (spctl & 0x10) && sprite_type >= 2 && sprite_type <= 7 &&
                                 (sprite_data & 0x8000) && !(spctl & 0x20);
            if (!windowed(wctl[L_SPRITE], win, x, sprite_window)) {
                switch (sprite_pixel(sprite_data, l[L_SPRITE])) {
                case S_DOT: consider(L_SPRITE); break;
                case S_SHADOW: shadow = l[L_SPRITE].prio; l[L_SPRITE] = {}; break;
                case S_NONE: break;
                }
            }
            auto consider_bg = [&](int layer, unsigned hardware_bit, const Nbg& s, Pix pixel) {
                Pix& p = l[layer];
                p = pixel;
                if (!s.on || !s.prio || !p.prio || windowed(wctl[layer], win, x, sprite_window)) return;
                p.prio = s.prio;
                if ((sfprmd >> (hardware_bit * 2) & 3) == 1) p.prio = (uint8_t)((s.prio & 6) | p.spr);
                uint8_t codes = (uint8_t)(reg(0x26) >> ((reg(0x24) >> hardware_bit & 1) * 8));
                bool code_match = codes >> p.code & 1;
                if ((sfprmd >> (hardware_bit * 2) & 3) == 2)
                    p.prio = (uint8_t)((s.prio & 6) | (!p.direct && p.spr && code_match));
                if (!p.prio) return;
                switch (sfccmd >> (hardware_bit * 2) & 3) {
                case 1: p.cc = s.cc && p.scc; break;
                case 2: p.cc = s.cc && p.scc && code_match; break;
                case 3: p.cc = s.cc && p.msb; break;
                default: p.cc = s.cc; break;
                }
                p.ratio = s.ratio; p.offset = s.offset; p.offset_b = s.offset_b;
                consider(layer);
            };
            l[L_RBG0] = {};
            if (rbg0_on) consider_bg(L_RBG0, 4, rbg[0], rotated[0][x]);
            for (int n = 0; n < 4; ++n) {
                Pix p{};
                // Preservar o filtro antecipado: pixels invisíveis não precisam
                // buscar padrões/cores. O pré-passe de scroll continua separado.
                if (!nbg[n].on || !nbg[n].prio || windowed(wctl[L_NBG0+n],win,x,sprite_window)) {
                    l[L_NBG0+n] = {};
                    continue;
                }
                if (n == 0 && rbg1_on) p = rotated[1][x];
                else if (nbg[n].on && nbg_pixel(nbg[n], x, y, p, n < 2 && !cells[n].empty() ? cells[n][x] : 0)) p.prio = 1;
                consider_bg(L_NBG0 + n, unsigned(n), n == 0 && rbg1_on ? rbg1 : nbg[n], p);
            }
            const Pix& t = top >= 0 ? l[top] : back;
            Pix u = second >= 0 ? l[second] : back;
            if (extended && u.cc) u.rgb = average(u.rgb, third >= 0 ? l[third].rgb : back.rgb);
            int top_bit = top == L_SPRITE ? 5 : top == L_RBG0 ? 4 : top - L_NBG0;
            if (top >= 0 && (lnclen >> top_bit & 1) && grad < 0) {
                Pix line_pixel = line_colour;
                if (t.coefficient_line) line_pixel.rgb = t.line_rgb;
                if (extended && line_pixel.cc) { u.rgb = average(u.rgb, line_pixel.rgb); u.ratio = line_pixel.ratio; }
                else u = line_pixel;
            }
            if (grad >= 0 && (top == grad || second == grad)) u.rgb = blur[x];
            uint32_t rgb = t.rgb;
            if (top >= 0 && t.cc && !windowed(cc_wctl, win, x, sprite_window)) {
                uint32_t out = 0;
                int ratio = second_ratio ? u.ratio : t.ratio;
                for (int sh = 16; sh >= 0; sh -= 8) {
                    int a = (int)(t.rgb >> sh & 0xFF), b = (int)(u.rgb >> sh & 0xFF);
                    // ratio 0 is 31:1, not 32:0: the second image always shows through a little
                    int c = add ? std::min(a + b, 255) : (a * (31 - ratio) + b * (ratio + 1)) >> 5;
                    out |= (uint32_t)c << sh;
                }
                rgb = out;
            }
            if (t.offset) rgb = apply_offset(rgb, t.offset_b);
            // A shadow halves the top screen under it when SDCTL enables that screen (bit 5: the back screen).
            bool shaded = top == L_SPRITE ? t.shade
                        : shadow && shadow >= t.prio && (sdctl >> (top >= 0 ? top_bit : 5) & 1);
            if (shaded) rgb = rgb >> 1 & 0x7F7F7F;
            f.px[(size_t)y * f.w + x] = rgb;
        }
    }
}

void vdp2_state(State& s) { s(g_vdp2_vram), s(g_vdp2_cram), s(g_vdp2_regs), s(g_vcell_last); }
