#pragma once

#include <array>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

// Observador do tempo real: nunca altera VRAM, relógio virtual ou regras do desenho.
namespace VideoPerf {

enum class Stage { Vdp1, Vdp2, Upload, Draw, Present, Devices, Sound, Cd, Slave, Pace, Master, GpuCompose, GpuWait, Vblank, Count };

inline int& runtime_override() { static int value = -1; return value; }
inline bool& paused() { static bool value = false; return value; }
inline bool enabled() {
    if (runtime_override() >= 0) return runtime_override() != 0;
    static const bool active = [] {
#if defined(_WIN32)
        char* value = nullptr;
        std::size_t size = 0;
        if (_dupenv_s(&value, &size, "SOTN_VIDEO_PROFILE") != 0) return false;
        const bool result = value && value[0] && std::strcmp(value, "0") != 0;
        std::free(value);
        return result;
#else
        const char* value = std::getenv("SOTN_VIDEO_PROFILE");
        return value && value[0] && std::strcmp(value, "0") != 0;
#endif
    }();
    return active;
}

namespace detail {
using Clock = std::chrono::steady_clock;
struct Sample {
    std::uint64_t count = 0;
    double totalMs = 0;
    double maxMs = 0;
    // 16 divisões por potência de dois em microssegundos; memória limitada.
    std::array<std::uint64_t, 1025> histogram{};
    void add(double ms) {
        if (!std::isfinite(ms) || ms < 0) return;
        ++count;
        totalMs += ms;
        if (ms > maxMs) maxMs = ms;
        const double us = std::ceil(ms * 1000);
        const std::uint64_t value = us >= 0x1p64 ? UINT64_MAX : static_cast<std::uint64_t>(us);
        std::size_t bucket = 0;
        if (value) {
            const unsigned exponent = static_cast<unsigned>(std::bit_width(value) - 1);
            const auto remainder = value - (std::uint64_t{1} << exponent);
            const auto fraction = exponent >= 4 ? remainder >> (exponent - 4) : remainder << (4 - exponent);
            bucket = 1 + exponent * 16 + static_cast<std::size_t>(fraction);
        }
        ++histogram[bucket];
    }
    double percentile(unsigned percent) const {
        if (!count) return 0;
        const std::uint64_t target = count / 100 * percent + (count % 100 * percent + 99) / 100;
        std::uint64_t accumulated = 0;
        for (std::size_t i = 0; i < histogram.size(); ++i) {
            accumulated += histogram[i];
            if (accumulated < target) continue;
            if (!i) return 0;
            const unsigned exponent = static_cast<unsigned>((i - 1) / 16);
            const double upper = std::ldexp(1.0 + static_cast<double>((i - 1) % 16 + 1) / 16, exponent) / 1000;
            return std::min(upper, maxMs);
        }
        return maxMs;
    }
};
struct State {
    std::array<Sample, static_cast<std::size_t>(Stage::Count)> samples{};
    Clock::time_point from = Clock::now();
    std::uint64_t lastVblank = 0;
    unsigned presentations = 0;
};
inline State& state() { static State value; return value; }
inline std::uint64_t& epoch() { static std::uint64_t value = 0; return value; }
inline std::string& last_report() { static std::string value; return value; }
} // namespace detail

inline void set_enabled(bool active, std::uint64_t vblank = 0) {
    ++detail::epoch();
    runtime_override() = active ? 1 : 0;
    detail::state() = {};
    detail::state().lastVblank = vblank;
    detail::last_report().clear();
}

inline bool repeat_last_report() {
    if (detail::last_report().empty()) return false;
    std::fputs(detail::last_report().c_str(), stderr);
    return true;
}

inline void reset_window(std::uint64_t vblank) {
    ++detail::epoch();
    detail::state() = {};
    detail::state().lastVblank = vblank;
}

// Uso no thread da máquina/renderer. Escopos aninhados são inclusivos, não somar suas médias.
class Scope {
public:
    explicit Scope(Stage stage) : stage_(stage), active_(enabled() && !paused() && stage >= Stage::Vdp1 && stage < Stage::Count) {
        if (active_) {
            detail::state();
            epoch_ = detail::epoch();
            from_ = detail::Clock::now();
        }
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    ~Scope() {
        if (!active_ || epoch_ != detail::epoch()) return;
        const double ms = std::chrono::duration<double, std::milli>(detail::Clock::now() - from_).count();
        auto& sample = detail::state().samples[static_cast<std::size_t>(stage_)];
        sample.add(ms);
    }
private:
    Stage stage_;
    bool active_;
    std::uint64_t epoch_ = 0;
    detail::Clock::time_point from_{};
};

// Mede o intervalo fora dos polls: CPU mestre e interrupções, sem dispositivos.
class MasterGap {
    struct Mark { detail::Clock::time_point from{}; std::uint64_t epoch = 0; bool ready = false; };
    static Mark& mark() { static Mark value; return value; }
public:
    MasterGap() {
        auto& m = mark();
        if (m.ready && m.epoch == detail::epoch() && enabled() && !paused()) {
            const double ms = std::chrono::duration<double, std::milli>(detail::Clock::now() - m.from).count();
            detail::state().samples[static_cast<std::size_t>(Stage::Master)].add(ms);
        }
        m.ready = false;
    }
    ~MasterGap() {
        auto& m = mark();
        m.ready = enabled() && !paused();
        if (m.ready) { m.from = detail::Clock::now(); m.epoch = detail::epoch(); }
    }
    MasterGap(const MasterGap&) = delete;
    MasterGap& operator=(const MasterGap&) = delete;
};

inline void mark_vblank() {
    static detail::Clock::time_point from{};
    static std::uint64_t epoch = 0;
    static bool ready = false;
    if (!enabled() || paused()) { ready = false; return; }
    const auto now = detail::Clock::now();
    if (ready && epoch == detail::epoch()) {
        detail::state().samples[static_cast<std::size_t>(Stage::Vblank)].add(
            std::chrono::duration<double, std::milli>(now - from).count());
    }
    from = now; epoch = detail::epoch(); ready = true;
}

// Chamar uma vez por apresentação; force também entrega a janela parcial ou um run headless.
inline void report(std::uint64_t vblank, int width, int height, bool force = false) {
    if (!enabled() || paused()) return;
    auto& state = detail::state();
    if (!force && ++state.presentations < 300) return;
    const auto now = detail::Clock::now();
    const double seconds = std::chrono::duration<double>(now - state.from).count();
    const std::uint64_t elapsedVblanks = vblank >= state.lastVblank ? vblank - state.lastVblank : 0;
    const double rate = seconds > 0 ? static_cast<double>(elapsedVblanks) / seconds : 0;
    char buffer[512];
    std::snprintf(buffer, sizeof buffer, "VIDEO_PROFILE vb=%llu size=%dx%d wall_s=%.6f vb_s=%.3f",
                 static_cast<unsigned long long>(vblank), width, height, seconds, rate);
    std::string line = buffer;
    static const std::array<const char*, static_cast<std::size_t>(Stage::Count)> names{
        "vdp1", "vdp2", "upload", "draw", "present", "devices", "sound", "cd", "slave", "pace", "master", "gpu_compose", "gpu_wait", "vblank"};
    for (std::size_t i = 0; i < state.samples.size(); ++i) {
        const auto& sample = state.samples[i];
        std::snprintf(buffer, sizeof buffer, " %s(count=%llu,avg_ms=%.6f,max_ms=%.6f) %s_pct(p50_ms=%.6f,p95_ms=%.6f,p99_ms=%.6f)", names[i],
                     static_cast<unsigned long long>(sample.count),
                     sample.count ? sample.totalMs / static_cast<double>(sample.count) : 0, sample.maxMs, names[i],
                     sample.percentile(50), sample.percentile(95), sample.percentile(99));
        line += buffer;
    }
    line += '\n';
    detail::last_report() = line;
    std::fputs(line.c_str(), stderr);
    state.samples = {};
    state.from = now;
    state.lastVblank = vblank;
    state.presentations = 0;
}

} // namespace VideoPerf
