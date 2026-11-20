#include "sotn_video.h"

#include <cassert>
#include <climits>
#include <cstdio>

using namespace SotnVideo;

static void expectRect(Rect actual, int x, int y, int w, int h) {
    assert(actual.x == x && actual.y == y && actual.w == w && actual.h == h);
}

int main() {
    static_assert(kResolutions.size() == 8);
    static_assert(kWindowScales == std::array<unsigned, 6>{{1, 2, 3, 4, 5, 6}});
    static_assert(kSpeeds == std::array<unsigned, 6>{{1, 2, 4, 6, 8, 10}});
    assert(kResolutions[2].width == 1280 && kResolutions[2].height == 720);
    assert(kResolutions[4].width == 1920 && kResolutions[4].height == 1080);
    assert(kEffectNames.size() == 3);

    expectRect(viewport(1920, 1080, 320, 224, false), 240, 0, 1440, 1080);
    expectRect(viewport(1280, 720, 320, 224, false), 160, 0, 960, 720);
    expectRect(viewport(640, 480, 320, 224, false), 0, 0, 640, 480);
    expectRect(viewport(1080, 1920, 320, 224, false), 0, 555, 1080, 810);
    expectRect(viewport(1280, 720, 320, 224, true), 160, 24, 960, 672);
    expectRect(viewport(100, 100, 320, 224, true), 0, 15, 100, 70);
    expectRect(viewport(1280, 720, 320, 224, false, 1), 126, 0, 1028, 720);
    expectRect(viewport(1280, 720, 320, 224, false, 2), 0, 0, 1280, 720);
    expectRect(viewport(1280, 720, 320, 224, true, 2), 0, 0, 1280, 720);
    expectRect(viewport(1920, 1080, 320, 224, false, 3), 0, 0, 1920, 1080);
    expectRect(viewport(1920, 1080, 320, 224, true, 3), 0, 0, 1920, 1080);
    expectRect(viewport(1920, 1080, 320, 224, false, 4), 96, 0, 1728, 1080);
    expectRect(viewport(1920, 1080, 320, 224, false, 5), 0, 129, 1920, 822);
    expectRect(viewport(1920, 1080, 320, 224, false, 6), 150, 0, 1620, 1080);
    expectRect(viewport(1920, 1080, 320, 224, false, 7), 285, 0, 1350, 1080);
    expectRect(viewport(1920, 1080, 320, 224, false, 8), 420, 0, 1080, 1080);
    expectRect(viewport(1, 1, 320, 224, true), 0, 0, 1, 1);
    expectRect(viewport(0, 720, 320, 224, true), 0, 0, 0, 0);
    expectRect(viewport(1280, -1, 320, 224, false), 0, 0, 0, 0);
    expectRect(viewport(1280, 720, 0, 224, true), 0, 0, 0, 0);
    const Rect large = viewport(INT_MAX, INT_MAX, INT_MAX, INT_MAX - 1, true);
    assert(large.w == INT_MAX && large.h == INT_MAX - 1);
    for (bool integer : {false, true}) {
        for (int w : {1, 2, 100, 319, 320, 640, 1280, INT_MAX}) {
            for (int h : {1, 2, 100, 223, 224, 480, 720, INT_MAX}) {
                Rect r = viewport(w, h, 320, 224, integer);
                assert(r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0);
                assert(std::int64_t(r.x) + r.w <= w);
                assert(std::int64_t(r.y) + r.h <= h);
            }
        }
    }

    assert(effect(0xAB123456u, 0, 0, Effect::Original) == 0xAB123456u);
    assert(effect(0xABFFFFFFu, 0, 0, Effect::Scanlines) == 0xABFFFFFFu);
    assert(effect(0xABFFFFFFu, 0, 1, Effect::Scanlines) == 0xABBFBFBFu);
    assert(effect(0xABFFFFFFu, 0, 0, Effect::Lcd) == 0xABFFDBDBu);
    assert(effect(0xABFFFFFFu, 1, 0, Effect::Lcd) == 0xABDBFFDBu);
    assert(effect(0xABFFFFFFu, 2, 0, Effect::Lcd) == 0xABDBDBFFu);
    assert(effect(0xABFFFFFFu, -1, 0, Effect::Lcd) == effect(0xABFFFFFFu, 2, 0, Effect::Lcd));
    assert(effect(0xAB000000u, 0, 1, Effect::Scanlines) == 0xAB000000u);
    std::puts("PASS: video options, viewport and pixel effects");
}
