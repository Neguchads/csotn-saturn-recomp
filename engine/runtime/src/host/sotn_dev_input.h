#pragma once
#include "host.h"
#include <cstdint>

namespace SotnDevInput {

// Start espera brevemente pelo R3. Um toque sozinho ainda gera um pulso normal.
// O latch só rearma depois de soltar os dois, inclusive ao fechar o menu.
class MenuChord {
public:
    struct Sample { bool open_menu = false; bool guest_start = false; };
    static constexpr uint64_t kChordGraceMs = 180;
    static constexpr uint64_t kTapPulseMs = 35;

    Sample sample(bool start, bool r3, uint64_t now_ms, bool l3 = false) {
        Sample result;
        if (consumed_) {
            if (!start && !r3 && !l3) consumed_ = false;
            start_was_down_ = start;
            return result;
        }
        if (l3 && r3) {
            consumed_ = true;
            start_was_down_ = start;
            tap_until_ = 0;
            result.open_menu = true;
            return result;
        }
        if (start && !start_was_down_) start_from_ = now_ms;
        if (!start && start_was_down_ && now_ms - start_from_ < kChordGraceMs)
            tap_until_ = now_ms + kTapPulseMs;
        result.guest_start = (start && now_ms - start_from_ >= kChordGraceMs) || now_ms < tap_until_;
        start_was_down_ = start;
        return result;
    }

    bool consumed() const { return consumed_; }

private:
    bool consumed_ = false;
    bool start_was_down_ = false;
    uint64_t start_from_ = 0, tap_until_ = 0;
};

// O Saturn JP abre o mapa pelo menu Start e então L. L+R transforma em névoa.
// Só injeta botões; o chamador confirma o estado real do jogo a cada VBlank.
class MapShortcut {
public:
    static constexpr uint64_t kPulseVblanks = 4;
    static constexpr uint64_t kTimeoutVblanks = 600;

    bool request(uint64_t vblank, bool allowed, bool menu_ready) {
        if (!allowed || active()) return false;
        phase_ = menu_ready ? Phase::Left : Phase::Start;
        request_from_ = phase_from_ = vblank;
        return true;
    }

    uint16_t buttons(uint64_t vblank, bool menu_ready) {
        if (!active()) return 0;
        if (vblank < request_from_ || vblank - request_from_ >= kTimeoutVblanks) {
            cancel();
            return 0;
        }
        switch (phase_) {
        case Phase::Start:
            if (vblank - phase_from_ < kPulseVblanks) return B_START;
            phase_ = Phase::Wait;
            phase_from_ = vblank;
            return 0; // Soltar Start antes de L, mesmo se o menu já estiver pronto.
        case Phase::Wait:
            if (menu_ready && vblank > phase_from_) {
                phase_ = Phase::Left;
                phase_from_ = vblank;
                return B_L;
            }
            return 0;
        case Phase::Left:
            if (vblank - phase_from_ < kPulseVblanks && menu_ready) return B_L;
            cancel();
            return 0;
        case Phase::Idle: return 0;
        }
        return 0;
    }

    bool active() const { return phase_ != Phase::Idle; }
    void cancel() { phase_ = Phase::Idle; }

private:
    enum class Phase { Idle, Start, Wait, Left };
    Phase phase_ = Phase::Idle;
    uint64_t request_from_ = 0, phase_from_ = 0;
};
} // namespace SotnDevInput
