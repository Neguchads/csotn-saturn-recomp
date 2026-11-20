#include "video_perf.h"
#include <cassert>
#include <chrono>
#include <thread>

int main() {
    VideoPerf::detail::Sample sample;
    assert(sample.percentile(50) == 0);
    for (int i = 1; i <= 100; ++i) sample.add(i);
    assert(sample.count == 100 && sample.maxMs == 100);
    assert(sample.percentile(50) >= 50 && sample.percentile(50) <= 53.125);
    assert(sample.percentile(95) >= 95 && sample.percentile(95) <= 100);
    assert(sample.percentile(99) >= 99 && sample.percentile(99) <= 100);
    sample.add(-1);
    assert(sample.count == 100);
    sample = {};
    sample.add(0); assert(sample.percentile(99) == 0);
    sample.add(0.001); assert(sample.percentile(99) == 0.001);
    VideoPerf::set_enabled(true);
    VideoPerf::mark_vblank(); VideoPerf::mark_vblank();
    const auto vblank = static_cast<std::size_t>(VideoPerf::Stage::Vblank);
    assert(VideoPerf::detail::state().samples[vblank].count == 1);
    VideoPerf::reset_window(0); VideoPerf::mark_vblank();
    assert(VideoPerf::detail::state().samples[vblank].count == 0);
    { VideoPerf::MasterGap first; }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    { VideoPerf::MasterGap second; }
    const auto master = static_cast<std::size_t>(VideoPerf::Stage::Master);
    assert(VideoPerf::detail::state().samples[master].count == 1);
    VideoPerf::reset_window(1);
    { VideoPerf::MasterGap reset; }
    assert(VideoPerf::detail::state().samples[master].count == 0);
    { VideoPerf::Scope old(VideoPerf::Stage::Devices); VideoPerf::reset_window(2); }
    assert(VideoPerf::detail::state().samples[static_cast<std::size_t>(VideoPerf::Stage::Devices)].count == 0);
    { VideoPerf::Scope enabled(VideoPerf::Stage::Sound); VideoPerf::set_enabled(false); VideoPerf::set_enabled(true); }
    assert(VideoPerf::detail::state().samples[static_cast<std::size_t>(VideoPerf::Stage::Sound)].count == 0);
    sample = {};
    sample.add(100000); assert(sample.percentile(99) == 100000);
    VideoPerf::detail::state().samples[master] = sample;
    VideoPerf::report(3, 320, 240, true);
    assert(VideoPerf::detail::last_report().find("p99_ms=") != std::string::npos);
    assert(VideoPerf::detail::last_report().find("master(count=1") != std::string::npos);
}
