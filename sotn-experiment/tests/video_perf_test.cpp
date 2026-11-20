#include "video_perf.h"
#include <cassert>
#include <chrono>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    const bool expected = argc > 1 && std::string(argv[1]) == "enabled";
    assert(VideoPerf::enabled() == expected);
    for (int sample = 0; sample < 3; ++sample) {
        for (auto stage : {VideoPerf::Stage::Vdp1, VideoPerf::Stage::Vdp2, VideoPerf::Stage::Upload,
                           VideoPerf::Stage::Draw, VideoPerf::Stage::Present, VideoPerf::Stage::Devices,
                           VideoPerf::Stage::Sound, VideoPerf::Stage::Cd, VideoPerf::Stage::Slave,
                           VideoPerf::Stage::Pace}) {
            VideoPerf::Scope scope(stage);
            if (expected) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    for (int frame = 1; frame <= 300; ++frame) VideoPerf::report(frame, 320, 224);
    {
        VideoPerf::Scope scope(VideoPerf::Stage::Vdp2);
        if (expected) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    VideoPerf::report(301, 640, 480, true);
    if (expected) {
        // Um poll pode abrir o menu; sua amostra antiga não pertence à janela nova.
        {
            VideoPerf::Scope scope(VideoPerf::Stage::Devices);
            VideoPerf::reset_window(302);
        }
        const auto index = static_cast<std::size_t>(VideoPerf::Stage::Devices);
        assert(VideoPerf::detail::state().samples[index].count == 0);
        {
            VideoPerf::Scope scope(VideoPerf::Stage::Sound);
            VideoPerf::set_enabled(false, 302);
            VideoPerf::set_enabled(true, 302);
        }
        assert(VideoPerf::detail::state().samples[static_cast<std::size_t>(VideoPerf::Stage::Sound)].count == 0);
    }
}
