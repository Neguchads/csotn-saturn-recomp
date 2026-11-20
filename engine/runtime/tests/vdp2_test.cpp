// VDP2 composition from registers, VRAM and colour RAM set by hand.
#include "test.h"
#include "video.h"
#include <cstring>

namespace {

const uint32_t kRed = 0xF80000, kGreen = 0x00F800, kBlue = 0x0000F8;   // RGB555 0x001F, 0x03E0, 0x7C00

// VDP1 shows dot d in columns x0 to x1 of every line, and nothing elsewhere.
void show_sprites(uint16_t d = 0, int x0 = 0, int x1 = -1) {
    for (uint32_t i = 0; i < 512 * 256; ++i) vdp1_fb_write(i * 2, i % 512 >= (uint32_t)x0 && i % 512 <= (uint32_t)x1 ? d : 0, 2);
    vdp1_vblank_out();
}

void reset() {
    vdp2_reset_fetch();
    std::memset(g_vdp2_regs, 0, sizeof g_vdp2_regs);
    std::memset(g_vdp2_vram, 0, sizeof g_vdp2_vram);
    std::memset(g_vdp2_cram, 0, sizeof g_vdp2_cram);
    show_sprites();
}

void reg(uint32_t off, uint16_t v) { g_vdp2_regs[off] = v >> 8; g_vdp2_regs[off + 1] = (uint8_t)v; }
void vram16(uint32_t a, uint16_t v) { g_vdp2_vram[a] = v >> 8; g_vdp2_vram[a + 1] = (uint8_t)v; }
void vram32(uint32_t a, uint32_t v) { vram16(a, uint16_t(v >> 16)); vram16(a + 2, uint16_t(v)); }
void cram16(uint32_t i, uint16_t v) { g_vdp2_cram[i * 2] = v >> 8; g_vdp2_cram[i * 2 + 1] = (uint8_t)v; }

uint32_t at(const Frame& f, int x, int y) { return f.px[(size_t)y * f.w + x]; }

// 320x224 with the display on, and a red back screen at VRAM 0x40000.
void red_screen() {
    reset();
    reg(0x00, 0x8000);                           // TVMD: display on
    reg(0xAC, 0x0002);                           // BKTAU/BKTAL: back screen at word 0x20000
    vram16(0x40000, 0x001F);
}

// NBG0 as a 512x256 bitmap of 256 colours at VRAM 0, every dot colour 1 (green), priority 1.
void green_bitmap_nbg0() {
    reg(0x20, 0x0001);                           // BGON: NBG0
    reg(0x28, 0x0012);                           // CHCTLA: NBG0 bitmap, 256 colours, 512x256
    reg(0x78, 0x0001);                           // ZMXIN0: X increment 1.0
    reg(0x7C, 0x0001);                           // ZMYIN0: Y increment 1.0
    reg(0xF8, 0x0001);                           // PRINA: NBG0 priority 1
    std::memset(g_vdp2_vram, 1, 512 * 256);
    cram16(1, 0x03E0);
}

}  // namespace

TEST(cell_coordinates_cover_plane_page_map_wrap_and_flipped_subcells) {
    for (int layout = 0; layout < 4; ++layout)
    for (int large = 0; large < 2; ++large)
    for (int one_word = 0; one_word < 2; ++one_word) {
        red_screen();
        const int pages_x = layout == 0 ? 1 : 2, pages_y = layout == 3 ? 2 : 1;
        const int cell = large ? 16 : 8, count = 512 / cell;
        const int page_bytes = (large ? 0x800 : 0x2000) * (one_word ? 1 : 2);
        reg(0x20, 1); reg(0x28, uint16_t(large));
        reg(0x30, one_word ? 0x8008 : 0); reg(0x3A, uint16_t(layout));
        reg(0xF8, 1); reg(0x78, 1); reg(0x7C, 1);
        const unsigned step = unsigned(pages_x * pages_y);
        reg(0x40, uint16_t(step << 8));
        reg(0x42, uint16_t(step * 2 | (step * 3 << 8)));
        for (int plane = 0; plane < 4; ++plane)
        for (int page = 0; page < pages_x * pages_y; ++page)
        for (int cy = 0; cy < count; ++cy)
        for (int cx = 0; cx < count; ++cx) {
            unsigned tile = unsigned((plane * 13 + page * 7 + cy * 3 + cx) % 16);
            unsigned address = unsigned((plane * pages_x * pages_y + page) * page_bytes +
                                        (cy * count + cx) * (one_word ? 2 : 4));
            if (one_word) vram16(address, uint16_t(tile | (cx & 1 ? 0x400 : 0) | (cy & 1 ? 0x800 : 0)));
            else {
                vram16(address, uint16_t((cx & 1 ? 0x4000 : 0) | (cy & 1 ? 0x8000 : 0)));
                vram16(address + 2, uint16_t(0x2000 + tile * (large ? 4 : 1)));
            }
        }
        for (unsigned tile = 0; tile < 64; ++tile)
        for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; x += 2) {
            unsigned first = 1 + (tile + x + y * 3) % 15;
            unsigned second = 1 + (tile + x + 1 + y * 3) % 15;
            g_vdp2_vram[0x40000 + tile * 32 + y * 4 + x / 2] = uint8_t(first << 4 | second);
        }
        for (unsigned i = 1; i <= 15; ++i) cram16(i, uint16_t(i));
        for (int scroll : {0, 505, 1017, 2041}) {
            const int scroll_y = (scroll + 4) & 2047;
            reg(0x70, uint16_t(scroll)); reg(0x74, uint16_t(scroll_y));
            Frame frame; vdp2_compose(frame);
            unsigned errors = 0;
            // Oráculo por coordenadas da cena, independente do caminho de lookup.
            for (int y = 0; y < frame.h; ++y)
            for (int x = 0; x < frame.w; ++x) {
                int sx = (scroll + x) % (pages_x * 1024), sy = (scroll_y + y) % (pages_y * 1024);
                int plane = (sy / (pages_y * 512)) * 2 + sx / (pages_x * 512);
                int ix = sx % (pages_x * 512), iy = sy % (pages_y * 512);
                int page = (iy / 512) * pages_x + ix / 512;
                int cx = (ix % 512) / cell, cy = (iy % 512) / cell;
                int px = ix % cell, py = iy % cell;
                if (cx & 1) px = cell - 1 - px;
                if (cy & 1) py = cell - 1 - py;
                unsigned tile = unsigned((plane * 13 + page * 7 + cy * 3 + cx) % 16) * (large ? 4u : 1u);
                if (large) tile += unsigned((py / 8) * 2 + px / 8);
                unsigned colour = 1 + (tile + unsigned(px % 8) + unsigned(py % 8) * 3) % 15;
                if (at(frame, x, y) != (colour * 8 << 16)) ++errors;
            }
            CHECK_EQ(errors, 0);
        }
    }
}

TEST(cell_snapshot_refreshes_pattern_attributes_and_palette_between_compositions) {
    red_screen(); reg(0x20, 1); reg(0x30, 0x8000); reg(0xF8, 1);
    reg(0x78, 1); reg(0x7C, 1);
    vram16(0, 0x100); vram16(2, 0x100);
    std::memset(g_vdp2_vram + 0x2000, 0x11, 32);
    std::memset(g_vdp2_vram + 0x2020, 0x22, 32);
    cram16(1, 0x03E0); cram16(2, 0x7C00);
    Frame frame; vdp2_compose(frame);
    CHECK_EQ(at(frame, 0, 0), kGreen);
    vram16(0, 0x101); vdp2_compose(frame);
    CHECK_EQ(at(frame, 0, 0), kBlue);
    cram16(2, 0x001F); vdp2_compose(frame);
    CHECK_EQ(at(frame, 0, 0), kRed);
    reg(0x70, 8); vdp2_compose(frame);
    CHECK_EQ(at(frame, 0, 0), kGreen);
}

TEST(display_off_is_black) {
    reset();
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(f.w, 320);
    CHECK_EQ(f.h, 224);
    CHECK_EQ(at(f, 100, 100), 0);
}

TEST(back_screen_shows_with_no_layers) {
    red_screen();
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kRed);
    CHECK_EQ(at(f, 319, 223), kRed);
}

TEST(bitmap_dot_zero_is_transparent) {
    red_screen();
    green_bitmap_nbg0();
    g_vdp2_vram[5] = 0;                          // dot (5, 0)
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 4, 0), kGreen);
    CHECK_EQ(at(f, 5, 0), kRed);
    CHECK_EQ(at(f, 5, 1), kGreen);
}

TEST(window_inside_hides_the_layer) {
    red_screen();
    green_bitmap_nbg0();
    reg(0xC4, 38);                               // WPEX0: X 0-38 half dots, dots 0-19
    reg(0xC6, 223);                              // WPEY0
    reg(0xD0, 0x0002);                           // WCTLA: NBG0 uses window 0, transparent inside
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 100), kRed);
    CHECK_EQ(at(f, 19, 100), kRed);
    CHECK_EQ(at(f, 20, 100), kGreen);
}

TEST(window_outside_hides_the_rest) {
    red_screen();
    green_bitmap_nbg0();
    reg(0xC4, 38);
    reg(0xC6, 223);
    reg(0xD0, 0x0003);                           // WCTLA: window 0, transparent outside
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 100), kGreen);
    CHECK_EQ(at(f, 20, 100), kRed);
}

TEST(colour_calculation_mixes_by_ratio) {
    red_screen();
    green_bitmap_nbg0();
    reg(0xEC, 0x0001);                           // CCCTL: NBG0
    reg(0x108, 16);                              // CCRNA: NBG0 ratio 16, 15:17
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x837400);
}

TEST(colour_calculation_at_ratio_0_still_shows_the_second_image) {
    red_screen();
    green_bitmap_nbg0();
    reg(0xEC, 0x0001);                           // CCCTL: NBG0, ratio 0
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x07F000);
}

TEST(higher_priority_wins_and_nbg0_wins_ties) {
    red_screen();
    green_bitmap_nbg0();
    reg(0x20, 0x0003);                           // BGON: NBG0, NBG1
    reg(0x28, 0x1212);                           // CHCTLA: both bitmaps of 256 colours
    reg(0x3C, 0x0010);                           // MPOFN: NBG1's bitmap at 0x20000
    reg(0x88, 0x0001);                           // ZMXIN1
    reg(0x8C, 0x0001);                           // ZMYIN1
    std::memset(g_vdp2_vram + 0x20000, 2, 512 * 256);
    cram16(2, 0x7C00);
    Frame f;
    reg(0xF8, 0x0201);                           // NBG1 2 over NBG0 1
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);
    reg(0xF8, 0x0103);                           // NBG0 3 over NBG1 1
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    reg(0xF8, 0x0202);                           // a tie
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
}

// The sprite layer as type 2 (priority bit 14, colour bits 0-10) at priority 3, over green NBG0 at 1.
void sprites_over_green() {
    red_screen();
    green_bitmap_nbg0();
    reg(0xE0, 0x0002);                           // SPCTL: type 2
    reg(0xF0, 0x0003);                           // PRISA: sprite priority 0 is 3
}

TEST(sprite_16bit_readout_doubles_in_640_and_704_modes) {
    for (int mode : {2, 3}) {
        sprites_over_green();
        vdp1_reg_write(0, 0, 2);
        reg(0, 0x8000 | mode);
        cram16(2, 0x7C00);
        show_sprites(2, 10, 19);
        Frame f;
        vdp2_compose(f);
        CHECK_EQ(at(f, 19, 0), kGreen);
        CHECK_EQ(at(f, 20, 0), kBlue);
        CHECK_EQ(at(f, 39, 0), kBlue);
        CHECK_EQ(at(f, 40, 0), kGreen);
    }
}

TEST(sprite_16bit_readout_keeps_native_width_in_320_and_352_modes) {
    for (int mode : {0, 1}) {
        sprites_over_green();
        vdp1_reg_write(0, 0, 2);
        reg(0, 0x8000 | mode);
        cram16(2, 0x7C00);
        show_sprites(2, 10, 19);
        Frame f;
        vdp2_compose(f);
        CHECK_EQ(at(f, 9, 0), kGreen);
        CHECK_EQ(at(f, 10, 0), kBlue);
        CHECK_EQ(at(f, 19, 0), kBlue);
        CHECK_EQ(at(f, 20, 0), kGreen);
    }
}

TEST(sprite_8bit_readout_skips_alternate_pixels_in_low_resolution) {
    sprites_over_green();
    vdp1_reg_write(0, 1, 2);
    cram16(2, 0x7C00);
    // Cada palavra contém dois pontos: azul par, transparente ímpar.
    show_sprites(0x0200, 10, 19);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 9, 0), kGreen);
    CHECK_EQ(at(f, 10, 0), kBlue);
    CHECK_EQ(at(f, 19, 0), kBlue);
    CHECK_EQ(at(f, 20, 0), kGreen);
    vdp1_reg_write(0, 0, 2);
}

TEST(normal_shadow_halves_an_enabled_screen) {
    sprites_over_green();
    show_sprites(0x07FE, 10, 19);                // every colour bit 1 but the lowest
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), kGreen);              // SDCTL enables no screen
    reg(0xE2, 0x0001);                           // SDCTL: NBG0
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), 0x007C00);
    CHECK_EQ(at(f, 20, 0), kGreen);
    reg(0x20, 0x0000);                           // BGON: nothing, so the back screen is on top
    reg(0xE2, 0x0020);                           // SDCTL: the back screen
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), 0x7C0000);
}

TEST(shadow_needs_a_priority_over_the_screen) {
    sprites_over_green();
    reg(0xE2, 0x0001);
    reg(0xF8, 0x0004);                           // PRINA: NBG0 4, over the shadow's 3
    show_sprites(0x07FE, 10, 19);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), kGreen);
}

TEST(msb_shadow_halves_the_sprite_itself) {
    sprites_over_green();
    cram16(2, 0x7C00);
    show_sprites(0x8002, 10, 19);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), 0x00007C);
    reg(0xE0, 0x0012);                           // SPCTL: SPWINEN, so the MSB is the sprite window's
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), kBlue);
}

TEST(transparent_shadow_needs_tpsdsl) {
    sprites_over_green();
    reg(0xE2, 0x0001);
    show_sprites(0x8000, 10, 19);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), kGreen);
    reg(0xE2, 0x0101);                           // SDCTL: TPSDSL and NBG0
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), 0x007C00);
}

// Green NBG0 mixed half and half, over a line colour table at VRAM 0x50000.
void line_colour_over_green() {
    red_screen();
    green_bitmap_nbg0();
    reg(0xEC, 0x0001);                           // CCCTL: NBG0
    reg(0x108, 16);                              // CCRNA: NBG0 ratio 16 of 32
    reg(0xAA, 0x8000);                           // LCTAL: the table at word 0x28000
    reg(0xA8, 0x0002);                           // LCTAU: one colour for the screen
    vram16(0x50000, 3);                          // the colour RAM entry for it
    cram16(3, 0x7C00);
}

TEST(line_colour_is_the_second_image_where_lnclen_names_the_top) {
    line_colour_over_green();
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x837400);             // LNCLEN off: green with the red back screen
    reg(0xE8, 0x0001);                           // LNCLEN: NBG0
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x007483);             // green with the blue line colour
}

TEST(line_colour_a_line) {
    line_colour_over_green();
    reg(0xE8, 0x0001);
    reg(0xA8, 0x8002);                           // LCTAU: LCCLMD, a colour a line
    vram16(0x50002, 4);                          // line 1's entry
    cram16(4, 0x001F);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x007483);
    CHECK_EQ(at(f, 0, 1), 0x837400);             // line 1: red
}

// Green NBG1 mixed half and half over NBG0, whose bitmap is blue for x 0-9 and black from x 10.
void green_over_blue_then_black() {
    red_screen();
    reg(0x20, 0x0003);                           // BGON: NBG0, NBG1
    reg(0x28, 0x1212);                           // CHCTLA: both bitmaps of 256 colours
    reg(0x3C, 0x0010);                           // MPOFN: NBG1's bitmap at 0x20000
    reg(0x78, 0x0001); reg(0x7C, 0x0001);        // ZMXIN0, ZMYIN0
    reg(0x88, 0x0001); reg(0x8C, 0x0001);        // ZMXIN1, ZMYIN1
    reg(0xF8, 0x0201);                           // PRINA: NBG1 2 over NBG0 1
    reg(0x108, 0x1000);                          // CCRNA: NBG1 ratio 16, 15:17
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 512; ++x) g_vdp2_vram[y * 512 + x] = x < 10 ? 2 : 5;
    std::memset(g_vdp2_vram + 0x20000, 1, 512 * 256);
    cram16(1, 0x03E0);
    cram16(2, 0x7C00);
    cram16(5, 0x0000);
}

TEST(gradation_blurs_the_second_image_along_the_line) {
    green_over_blue_then_black();
    reg(0xEC, 0x0002);                           // CCCTL: NBG1
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 10, 0), 0x007400);            // no gradation: half green, half black
    reg(0xEC, 0xA002);                           // CCCTL: BOKEN, BOKN NBG0, and NBG1
    vdp2_compose(f);
    CHECK_EQ(at(f, 5, 0), 0x007483);             // all blue around it
    CHECK_EQ(at(f, 10, 0), 0x007441);            // blue, blue, black: half blue
    CHECK_EQ(at(f, 11, 0), 0x007420);            // blue, black, black: a quarter
    CHECK_EQ(at(f, 12, 0), 0x007400);
}

TEST(exclusive_modes_keep_their_width_and_480_lines_without_double_counting_interlace) {
    for (int mode = 4; mode < 8; ++mode) {
        for (int lsmd : {0, 3}) {
            red_screen();
            reg(0, uint16_t(0x8000 | mode | (lsmd << 6)));
            Frame f;
            vdp2_compose(f);
            const int widths[] = {320, 352, 640, 704};
            CHECK_EQ(f.w, widths[mode & 3]);
            CHECK_EQ(f.h, 480);
            if (f.h == 480) CHECK_EQ(at(f, f.w - 1, 479), kRed);
        }
    }
}

TEST(special_priority_uses_the_original_dot_code_and_selected_code_bank) {
    red_screen();
    green_bitmap_nbg0();
    reg(0x2C, 0x20);                            // atributo de prioridade especial do bitmap
    reg(0xF8, 2);
    reg(0xEA, 2);
    reg(0x26, 0x0402);                          // banco A: código 1; banco B: código 2
    g_vdp2_vram[0] = 2;
    g_vdp2_vram[1] = 4;
    cram16(2, 0x03E0);
    cram16(4, 0x7C00);
    reg(0xE0, 0x20);
    reg(0xF0, 2);
    show_sprites(0x801F, 0, 10);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    CHECK_EQ(at(f, 1, 0), kRed);
    reg(0x24, 1);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kRed);
    CHECK_EQ(at(f, 1, 0), kBlue);
}

TEST(special_colour_by_dot_requires_both_the_attribute_and_code) {
    red_screen();
    green_bitmap_nbg0();
    reg(0x2C, 0x10);
    reg(0xEE, 2);
    reg(0x26, 0x0002);
    reg(0xEC, 1);
    reg(0x108, 31);
    g_vdp2_vram[0] = 2;
    g_vdp2_vram[1] = 4;
    cram16(2, 0x03E0);
    cram16(4, 0x7C00);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kRed);
    CHECK_EQ(at(f, 1, 0), kBlue);
    reg(0x2C, 0);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
}

TEST(sprite_window_masks_background_even_when_sprite_priority_is_zero) {
    red_screen();
    green_bitmap_nbg0();
    reg(0xE0, 0x12);                            // tipo 2, SPWINEN, sem RGB misto
    reg(0xD0, 0x20);                            // NBG0 transparente dentro da máscara
    show_sprites(0x8001, 4, 7);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 3, 0), kGreen);
    CHECK_EQ(at(f, 4, 0), kRed);
    CHECK_EQ(at(f, 7, 0), kRed);
    CHECK_EQ(at(f, 8, 0), kGreen);
    reg(0xD0, 0x30);                            // inversão da janela de sprite
    vdp2_compose(f);
    CHECK_EQ(at(f, 3, 0), kRed);
    CHECK_EQ(at(f, 4, 0), kGreen);
    reg(0xE0, 0x32);                            // RGB misto não gera máscara
    vdp2_compose(f);
    CHECK_EQ(at(f, 4, 0), kRed);
}

TEST(sprite_window_joins_rectangular_windows_and_masks_colour_calculation) {
    red_screen();
    green_bitmap_nbg0();
    reg(0xE0, 0x12);
    show_sprites(0x8001, 4, 9);
    reg(0xC4, 14); reg(0xC6, 223);             // janela 0: x0..7
    reg(0xD0, 0xA2);                           // AND, janela 0 e máscara de sprite
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 3, 0), kGreen);
    CHECK_EQ(at(f, 4, 0), kRed);
    CHECK_EQ(at(f, 8, 0), kGreen);
    reg(0xD0, 0);
    reg(0xEC, 1); reg(0x108, 31);
    reg(0xD6, 0x2000);                         // máscara apenas no cálculo de cor
    vdp2_compose(f);
    CHECK_EQ(at(f, 3, 0), kRed);
    CHECK_EQ(at(f, 4, 0), kGreen);
}

TEST(extended_colour_averages_second_and_third_only_when_second_requests_cc) {
    red_screen();
    green_bitmap_nbg0();
    reg(0x20, 3); reg(0x28, 0x1212); reg(0x3C, 0x10);
    reg(0x88, 1); reg(0x8C, 1); reg(0xF8, 0x0102);
    std::memset(g_vdp2_vram + 0x20000, 2, 512 * 256);
    cram16(2, 0x7C00);
    reg(0xE0, 0x0320); reg(0xF0, 3); reg(0x100, 15);
    show_sprites(0x801F, 0, 10);
    reg(0xEC, 0x0041);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x7C7C00);
    reg(0xEC, 0x0441);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x7C3E3E);
    reg(0xEC, 0x0440);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x7C7C00);
    reg(0xEC, 0x0441); reg(0, 0x8002);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), 0x7C7C00);
}

TEST(line_horizontal_scroll_obeys_each_table_interval_and_scroll_register) {
    for (int shift = 0; shift < 4; ++shift) {
        red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00);
        for (int y = 0; y < 256; ++y) g_vdp2_vram[y * 512 + 9] = 2;
        reg(0x70, 1);
        reg(0x9A, uint16_t(2 | (shift << 4)));
        reg(0xA0, 3);
        vram32(0x60000, 0);
        vram32(0x60004, 8u << 16);
        Frame f;
        vdp2_compose(f);
        CHECK_EQ(at(f, 0, 0), kGreen);
        CHECK_EQ(at(f, 0, (1 << shift) - 1), kGreen);
        CHECK_EQ(at(f, 0, 1 << shift), kBlue);
    }
}

TEST(mosaic_and_packed_line_scroll_keep_full_density_coordinates) {
    for (int density = 0; density < 2; ++density)
    for (int shift = 0; shift < 4; ++shift)
    for (int size : {1, 3, 16}) {
        red_screen(); green_bitmap_nbg0();
        cram16(1, 0x001F); cram16(2, 0x03E0); cram16(3, 0x7C00);
        for (int y=0; y<256; ++y) for (int x=0; x<512; ++x)
            g_vdp2_vram[y*512+x] = uint8_t((x/3+y/5)%3+1);
        if (density) reg(0, 0x80D0);
        const int horizontal = size == 3 ? 4 : size;
        reg(0x22, uint16_t(((size-1)<<12)|((horizontal-1)<<8)|1));
        reg(0x9A, uint16_t(14|(shift<<4))); reg(0xA0, 3);
        for (unsigned line=0; line<480; ++line) {
            vram32(0x60000+line*12, (line%7)<<16);
            vram32(0x60004+line*12, (line%11)<<16);
            vram32(0x60008+line*12, (1+line%3)<<15);
        }
        Frame f; vdp2_compose(f);
        unsigned errors = 0;
        const uint32_t colours[] = {kRed, kGreen, kBlue};
        // Oráculo da cena: aplica âncora de mosaico antes de selecionar a linha.
        for (int y=0; y<f.h; y+=9) for (int x=0; x<f.w; x+=7) {
            const unsigned my=unsigned(y-y%size), mx=unsigned(x-x%horizontal);
            const unsigned entry=my>>shift, step=(1+entry%3)*128;
            const unsigned sx=((entry%7)*256+mx*step)>>8;
            const unsigned sy=entry%11+(my&((1u<<shift)-1));
            const unsigned colour=((sx&511)/3+(sy&255)/5)%3;
            if (at(f,x,y)!=colours[colour]) ++errors;
        }
        CHECK_EQ(errors,0);
    }
}

TEST(line_vertical_scroll_restarts_its_accumulator_in_each_interval) {
    red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00);
    for (int y = 1; y < 256; y += 2) std::memset(g_vdp2_vram + y * 512, 2, 512);
    reg(0x9A, 0x14); reg(0xA0, 3);
    vram32(0x60000, 3u << 16);
    vram32(0x60004, 6u << 16);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);
    CHECK_EQ(at(f, 0, 1), kGreen);
    CHECK_EQ(at(f, 0, 2), kGreen);
    CHECK_EQ(at(f, 0, 3), kBlue);
}

TEST(line_scroll_and_zoom_use_only_enabled_fields_in_packed_tables) {
    red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00);
    reg(0x9A, 0x0E); reg(0xA0, 3);
    vram32(0x60000, 2u << 16);                  // X +2
    vram32(0x60004, 3u << 16);                  // Y 3
    vram32(0x60008, 2u << 16);                  // incremento X 2
    vram32(0x6000C, 4u << 16);                  // próxima linha: X4, Y5, incremento1
    vram32(0x60010, 5u << 16);
    vram32(0x60014, 1u << 16);
    g_vdp2_vram[3 * 512 + 4] = 2;
    g_vdp2_vram[5 * 512 + 5] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    CHECK_EQ(at(f, 1, 0), kBlue);
    CHECK_EQ(at(f, 1, 1), kBlue);
}

TEST(nbg1_has_an_independent_line_scroll_table) {
    red_screen();
    reg(0x20, 2); reg(0x28, 0x1200); reg(0x3C, 0x10);
    reg(0x88, 1); reg(0x8C, 1); reg(0xF8, 0x100);
    std::memset(g_vdp2_vram + 0x20000, 1, 512 * 256);
    cram16(1, 0x03E0); cram16(2, 0x7C00);
    g_vdp2_vram[0x20000 + 8] = 2;
    reg(0x9A, 0x200); reg(0xA4, 3); reg(0xA6, 0x20);
    vram32(0x60040, 8u << 16);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);
    CHECK_EQ(at(f, 1, 0), kGreen);
}

TEST(mosaic_replicates_screen_groups_before_scroll_and_fractional_zoom) {
    red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00); cram16(3, 0x7FFF);
    reg(0x70, 1); reg(0x7A, 0x8000);           // origem1, incremento1.5
    g_vdp2_vram[5] = 3;
    std::memset(g_vdp2_vram + 2 * 512, 2, 512);
    reg(0x22, 0x1201);                         // NBG0, H3, V2
    Frame f;
    vdp2_compose(f);
    for (int x = 0; x < 3; ++x) {
        CHECK_EQ(at(f, x, 0), kGreen);
        CHECK_EQ(at(f, x, 1), kGreen);
        CHECK_EQ(at(f, x, 2), kBlue);
    }
    CHECK_EQ(at(f, 3, 0), 0xF8F8F8);
    CHECK_EQ(at(f, 5, 1), 0xF8F8F8);
}

namespace {
void identity_rotation(int parameter) {
    uint32_t table = 0x70000 + uint32_t(parameter) * 0x80;
    vram32(table + 0x10, 1u << 16);             // deltaYst = 1.0
    vram32(table + 0x14, 1u << 16);             // deltaX = 1.0
    vram32(table + 0x1C, 1u << 16);             // matriz A=E=1.0
    vram32(table + 0x2C, 1u << 16);
    vram32(table + 0x4C, 1u << 16);             // kx=ky=1.0
    vram32(table + 0x50, 1u << 16);
}

void rotating_bitmaps() {
    red_screen();
    reg(0xAC, 3); reg(0xAE, 0xFF80); vram16(0x7FF00, 0x1F);
    reg(0x20, 0x10); reg(0x2A, 0x1200);        // RBG0, bitmap256 cores
    reg(0x3E, 0x21); reg(0xFC, 2);
    std::memset(g_vdp2_vram + 0x20000, 1, 512 * 256);
    std::memset(g_vdp2_vram + 0x40000, 2, 512 * 256);
    cram16(1, 0x03E0); cram16(2, 0x7C00);
    reg(0xBC, 3); reg(0xBE, 0x8000);
    identity_rotation(0); identity_rotation(1);
}
}

TEST(rotation_bitmap_identity_translation_and_fractional_scale) {
    rotating_bitmaps();
    g_vdp2_vram[0x20000 + 2] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    CHECK_EQ(at(f, 2, 0), kBlue);
    vram32(0x70000, 2u << 16);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);
    vram32(0x70000, 0); vram32(0x7004C, 0x8000);
    vdp2_compose(f);
    CHECK_EQ(at(f, 3, 0), kGreen);
    CHECK_EQ(at(f, 4, 0), kBlue);
}

TEST(rotation_matrix_crosses_axes_without_unsigned_negative_coordinates) {
    rotating_bitmaps();
    vram32(0x7001C, 0); vram32(0x70020, 1u << 16); // X=y
    vram32(0x70028, 0xFFFF0000); vram32(0x7002C, 0);
    vram32(0x70048, 8u << 16);                 // Y=8-x
    g_vdp2_vram[0x20000 + 7 * 512 + 2] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 1, 2), kBlue);
    CHECK_EQ(at(f, 0, 2), kGreen);
}

TEST(rotation_parameter_selection_and_rotation_window_choose_a_or_b) {
    rotating_bitmaps();
    Frame f;
    reg(0xB0, 0); vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kGreen);
    reg(0xB0, 1); vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
    reg(0xB0, 3); reg(0xD6, 2); reg(0xC4, 14); reg(0xC6, 223);
    vdp2_compose(f);
    CHECK_EQ(at(f, 7, 0), kBlue);
    CHECK_EQ(at(f, 8, 0), kGreen);
}

TEST(rotation_screen_over_wraps_or_makes_negative_and_large_coordinates_transparent) {
    rotating_bitmaps();
    vram32(0x70000, 0xFFFF0000);
    g_vdp2_vram[0x20000 + 511] = 2;
    Frame f;
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
    reg(0x3A, 2u << 10);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kRed);
    vram32(0x70000, 0); vram32(0x70004, 256u << 16);
    reg(0x3A, 3u << 10);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kGreen);
    vram32(0x70004, 512u << 16);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kRed);
}

TEST(rotation_coefficient_32bit_controls_scale_transparency_and_parameter_fallback) {
    rotating_bitmaps();
    reg(0x0E, 0x10);                            // banco B contém coeficientes
    reg(0xB4, 1); reg(0xB6, 1);
    vram32(0x70054, 0x80000000);                // índice0x18000 -> byte0x60000
    vram32(0x7005C, 1u << 16);
    vram32(0x60000, 0x10000);                  // x0: escala1
    vram32(0x60004, 0x20000);                  // x1: escala2
    vram32(0x60008, 0x80010000);               // x2: transparente
    g_vdp2_vram[0x20000 + 2] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    CHECK_EQ(at(f, 1, 0), kBlue);
    CHECK_EQ(at(f, 2, 0), kRed);
    reg(0xB0, 2);
    vdp2_compose(f); CHECK_EQ(at(f, 2, 0), kBlue);
}

TEST(rotation_coefficient_16bit_and_cram_storage_follow_their_own_formats) {
    rotating_bitmaps();
    reg(0x0E, 0x8010); reg(0xB4, 3); reg(0xB6, 0);
    vram32(0x70054, 0); vram32(0x7005C, 1u << 16);
    cram16(0x400, 0x400);                      // CRAM0x800: coeficiente1.0
    cram16(0x401, 0x800);                      // CRAM0x802: coeficiente2.0
    cram16(0x402, 0x8000);                     // CRAM0x804: transparente
    g_vdp2_vram[0x20000 + 2] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    CHECK_EQ(at(f, 1, 0), kBlue);
    CHECK_EQ(at(f, 2, 0), kRed);
}

TEST(rbg1_uses_parameter_b_and_nbg0_attributes_and_high_resolution_replicates_pairs) {
    rotating_bitmaps();
    reg(0x20, 0x20); reg(0x28, 0x12); reg(0xF8, 2);
    g_vdp2_vram[0x40001] = 1;
    reg(0, 0x8002);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);
    CHECK_EQ(at(f, 1, 0), kBlue);
    CHECK_EQ(at(f, 2, 0), kGreen);
    CHECK_EQ(at(f, 3, 0), kGreen);
}

TEST(rotation_uses_viewpoint_center_and_translation_in_the_same_fixed_point_domain) {
    rotating_bitmaps();
    vram32(0x7001C, 2u << 16);
    vram16(0x70034, 3); vram16(0x7003C, 4);
    vram32(0x70044, 6u << 16);
    g_vdp2_vram[0x20002] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);               // X=2*x+2, não 2*x+6
}

TEST(rotation_cell_maps_reach_planes_e_p_and_the_over_pattern) {
    rotating_bitmaps();
    reg(0x2A, 0); reg(0x38, 0x8000); reg(0x3E, 0);
    reg(0x50, 2); reg(0x54, 3); reg(0x5E, 0x400);
    std::memset(g_vdp2_vram + 0x2000, 0x11, 32);
    std::memset(g_vdp2_vram + 0x2020, 0x22, 32);
    std::memset(g_vdp2_vram + 0x2040, 0x33, 32);
    cram16(3, 0x7FFF);
    vram16(0x4000, 0x100); vram16(0x6000, 0x101); vram16(0x8000, 0x102);
    Frame f;
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kGreen);
    vram32(0x70004, 512u << 16);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
    vram32(0x70000, 1536u << 16); vram32(0x70004, 1536u << 16);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), 0xF8F8F8);
    vram32(0x70000, 2048u << 16); vram32(0x70004, 0);
    reg(0x3A, 1u << 10); reg(0xB8, 0x101);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
    reg(0x3A, 1u << 8);                        // plano2x1: X512 ainda é página do plano A
    vram32(0x70000, 512u << 16);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
}

TEST(rotation_coefficient_modes_select_both_axes_one_axis_or_translation) {
    rotating_bitmaps(); reg(0x0E, 0x10); reg(0xB6, 1);
    vram32(0x70054, 0x80000000);
    g_vdp2_vram[0x20000 + 2 * 512 + 2] = 2;
    g_vdp2_vram[0x20000 + 1 * 512 + 2] = 3;
    cram16(3, 0x7FFF);
    Frame f;
    vram32(0x60000, 0x20000);
    reg(0xB4, 1); vdp2_compose(f); CHECK_EQ(at(f, 1, 1), kBlue);
    reg(0xB4, 5); vdp2_compose(f); CHECK_EQ(at(f, 1, 1), 0xF8F8F8);
    reg(0xB4, 9); vdp2_compose(f); CHECK_EQ(at(f, 1, 1), kGreen);
    vram32(0x60000, 2); g_vdp2_vram[0x20000 + 512 + 3] = 2;
    reg(0xB4, 13); vdp2_compose(f); CHECK_EQ(at(f, 1, 1), kBlue);
}

TEST(rotation_keeps_the_last_coefficient_when_crossing_into_a_bank_without_access) {
    rotating_bitmaps(); reg(0x0E, 0x101); reg(0xB4, 1); reg(0xB6, 0);
    vram32(0x70054, 0x7FFF0000); vram32(0x7005C, 1u << 16);
    vram32(0x1FFFC, 0x10000);
    g_vdp2_vram[0x20001] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
    CHECK_EQ(at(f, 1, 0), kBlue);
}

TEST(rotation_line_colour_and_coefficient_line_index_feed_normal_composition) {
    rotating_bitmaps(); reg(0xEC, 0x10); reg(0x10C, 31); reg(0xE8, 0x10);
    reg(0xA8, 3); reg(0xAA, 0); vram16(0x60000, 2);
    Frame f;
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
    reg(0x0E, 0x10); reg(0xB4, 0x11); reg(0xB6, 1);
    vram32(0x70054, 0x80000000); vram32(0x60000, 0x02010000);
    reg(0xAA, 0x2000); vram16(0x64000, 0x180); cram16(0x182, 0x7C00);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), kBlue);
}

TEST(rotation_gradation_blurs_the_rotation_layer_rather_than_skipping_it) {
    rotating_bitmaps();
    for (int x = 0; x < 10; ++x) g_vdp2_vram[0x20000 + x] = 2;
    reg(0xEC, 0x9010); reg(0x10C, 31);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 9, 0), kBlue);
    CHECK_EQ(at(f, 10, 0), 0x007C7C);
    CHECK_EQ(at(f, 11, 0), 0x00BA3E);
}

TEST(vertical_cell_scroll_advances_with_source_cells_and_uses_access_pattern_stride) {
    red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00);
    std::memset(g_vdp2_vram + 512, 2, 512);
    reg(0x9A, 1); reg(0x9C, 3); reg(0x10, 0xCFFF);
    vram32(0x60000, 1u << 16); vram32(0x60004, 2u << 16);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue);
    CHECK_EQ(at(f, 7, 0), kBlue);
    CHECK_EQ(at(f, 8, 0), kGreen);
    reg(0x70, 4); reg(0x7A, 0x8000);           // origem4 e zoom1.5: próxima célula no x3
    vdp2_compose(f);
    CHECK_EQ(at(f, 2, 0), kBlue);
    CHECK_EQ(at(f, 3, 0), kGreen);
}

TEST(vertical_cell_scroll_interleaves_enabled_accesses_and_repeats_late_first_fetch) {
    red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00);
    reg(0x20, 3); reg(0x28, 0x1212); reg(0x3C, 0x10);
    reg(0x88, 1); reg(0x8C, 1);
    std::memset(g_vdp2_vram + 512, 2, 512);
    std::memset(g_vdp2_vram + 0x20000, 1, 512 * 256);
    std::memset(g_vdp2_vram + 0x20200, 2, 512);
    reg(0x9A, 0x101); reg(0x9C, 3); reg(0x10, 0xCDFF);
    vram32(0x60000, 1u << 16); vram32(0x60004, 2u << 16);
    vram32(0x60008, 2u << 16); vram32(0x6000C, 1u << 16);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue); CHECK_EQ(at(f, 8, 0), kGreen);
    reg(0xF8, 0x100);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen); CHECK_EQ(at(f, 8, 0), kBlue);
    reg(0x20, 1); reg(0xF8, 1); reg(0x9A, 1); reg(0x10, 0xFFCF);
    vram32(0x60004, 2u << 16);
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kBlue); CHECK_EQ(at(f, 8, 0), kBlue);
    CHECK_EQ(at(f, 16, 0), kGreen);
}

TEST(double_density_line_scroll_uses_full_output_line_indices_in_the_snapshot_model) {
    red_screen(); green_bitmap_nbg0(); cram16(2, 0x7C00);
    for (int y = 0; y < 256; ++y) g_vdp2_vram[y * 512 + 8] = 2;
    reg(0, 0x80C0); reg(0x9A, 0x12); reg(0xA0, 3);
    vram32(0x60000, 0); vram32(0x60004, 8u << 16);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen); CHECK_EQ(at(f, 0, 1), kGreen);
    CHECK_EQ(at(f, 0, 2), kBlue); CHECK_EQ(at(f, 0, 3), kBlue);
}

TEST(rotation_without_coefficient_banks_reads_one_coefficient_per_line) {
    rotating_bitmaps(); reg(0xB4, 1); reg(0xB6, 1);
    vram32(0x70054, 0x80000000); vram32(0x7005C, 1u << 16);
    vram32(0x60000, 0x10000); vram32(0x60004, 0x80000000);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen); CHECK_EQ(at(f, 1, 0), kGreen);
}

TEST(rotation_window_keeps_and_logic_when_both_rectangles_are_enabled) {
    rotating_bitmaps(); reg(0xB0, 3); reg(0xD6, 0x8A);
    reg(0xC4, 30); reg(0xC6, 223);
    reg(0xC8, 16); reg(0xCC, 46); reg(0xCE, 223);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 7, 0), kGreen); CHECK_EQ(at(f, 8, 0), kBlue);
    CHECK_EQ(at(f, 15, 0), kBlue); CHECK_EQ(at(f, 16, 0), kGreen);
}

TEST(rotation_mosaic_repeats_complete_pixels_in_native_coordinates) {
    rotating_bitmaps(); reg(0x0E, 0x10); reg(0xB4, 1); reg(0xB6, 1);
    reg(0x22, 0x110);                          // RBG0 H2
    vram32(0x70054, 0x80000000); vram32(0x7005C, 1u << 16);
    vram32(0x60000, 0x10000); vram32(0x60004, 0x80000000);
    vram32(0x60008, 0x10000); vram32(0x6000C, 0x80000000);
    g_vdp2_vram[0x20002] = 2;
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen); CHECK_EQ(at(f, 1, 0), kGreen);
    CHECK_EQ(at(f, 2, 0), kBlue); CHECK_EQ(at(f, 3, 0), kBlue);
    reg(0, 0x8002); vdp2_compose(f);
    CHECK_EQ(at(f, 3, 0), kGreen); CHECK_EQ(at(f, 4, 0), kBlue);
    CHECK_EQ(at(f, 7, 0), kBlue);
}

TEST(extended_line_colour_uses_line_ratio_with_second_screen_ratio_mode) {
    rotating_bitmaps();
    reg(0xEC, 0x0670); reg(0xE8, 0x20); reg(0x10C, 0); reg(0x10E, 31);
    reg(0xA8, 3); reg(0xAA, 0); vram16(0x60000, 2);
    reg(0xE0, 0x0320); reg(0xF0, 3);
    show_sprites(0x801F, 0, 10);
    Frame f;
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), 0x3E3E7C);
    reg(0x10C, 15);
    vdp2_compose(f); CHECK_EQ(at(f, 0, 0), 0x3E3E7C);
}

TEST(rotation_fallback_keeps_coefficient_a_line_colour_when_b_supplies_pixels) {
    rotating_bitmaps(); reg(0xB0, 2); reg(0x0E, 0x10);
    reg(0xB4, 0x1111); reg(0xB6, 0x101);
    vram32(0x70054, 0x80000000); vram32(0x700D4, 0x80010000);
    vram32(0x60000, 0x82010000); vram32(0x60004, 0x03010000);
    reg(0xEC, 0x10); reg(0xE8, 0x10); reg(0x10C, 31);
    reg(0xA8, 3); reg(0xAA, 0x2000); vram16(0x64000, 0);
    cram16(2, 0x03E0); cram16(3, 0x1F);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen);
}

TEST(rotation_vertical_mosaic_repeats_coefficient_transparency_and_colour) {
    rotating_bitmaps(); reg(0x0E, 0x10); reg(0xB4, 1); reg(0xB6, 1);
    reg(0x22, 0x1010);                         // RBG0 V2
    vram32(0x70054, 0x80000000); vram32(0x70058, 1u << 16);
    vram32(0x60000, 0x10000); vram32(0x60004, 0x80000000);
    vram32(0x60008, 0x80000000);
    Frame f;
    vdp2_compose(f);
    CHECK_EQ(at(f, 0, 0), kGreen); CHECK_EQ(at(f, 0, 1), kGreen);
    CHECK_EQ(at(f, 0, 2), kRed); CHECK_EQ(at(f, 0, 3), kRed);
}
