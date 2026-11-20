#include "sotn_dev_layout.h"
#include "video_perf.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    for (const auto size : {std::pair{320,240}, {640,480}, {960,720}, {1920,1080}, {3840,2160}, {600,1000}}) {
        for (const auto counts : {std::pair{9u,8u}, {6u,12u}, {8u,7u}, {3u,4u}}) {
            const auto l = SotnDevLayout::make(size.first, size.second, counts.first, counts.second);
            assert(l.panel.w > size.first*.95f && l.panel.h > size.second*.94f);
            assert(l.u > 0 && std::isfinite(l.u));
            for (int column = 0; column < 2; ++column) {
                const auto count = column == 0 ? counts.first : counts.second;
                for (unsigned i = 0; i < count; ++i) {
                    const auto r = l.cell(column, i);
                    assert(r.x >= l.panel.x && r.y > l.title_y);
                    assert(r.x+r.w <= l.panel.x+l.panel.w && r.y+r.h < l.footer_y);
                    assert(r.w > 0 && r.h > 0);
                    if (i > 0) { const auto prior = l.cell(column,i-1); assert(r.y > prior.y+prior.h); }
                }
            }
            if (l.two) assert(l.cell(0,0).x+l.column < l.cell(1,0).x);
            else assert(l.cell(0,counts.first-1).y+l.row < l.cell(1,0).y);
        }
    }
    assert(SotnDevLayout::make(0,0,8,12).panel.w == 0);
    VideoPerf::set_enabled(true, 123);
    assert(VideoPerf::enabled() && VideoPerf::detail::state().lastVblank == 123);
    { VideoPerf::Scope scope(VideoPerf::Stage::Vdp2); }
    assert(VideoPerf::detail::state().samples[1].count == 1);
    VideoPerf::paused() = true;
    { VideoPerf::Scope scope(VideoPerf::Stage::Vdp2); }
    assert(VideoPerf::detail::state().samples[1].count == 1);
    VideoPerf::paused() = false;
    VideoPerf::reset_window(456);
    assert(VideoPerf::detail::state().samples[1].count == 0 && VideoPerf::detail::state().lastVblank == 456);
    VideoPerf::set_enabled(false, 789);
    { VideoPerf::Scope scope(VideoPerf::Stage::Vdp2); }
    assert(!VideoPerf::enabled() && VideoPerf::detail::state().samples[1].count == 0);
    std::puts("PASS: large responsive dev layout, contained rows, profiler runtime toggle");
}
