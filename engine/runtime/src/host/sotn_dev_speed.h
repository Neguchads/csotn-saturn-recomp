#pragma once
#include <cstdint>

namespace SotnDevSpeed {
constexpr unsigned normalized(unsigned speed) {
    return speed == 2 || speed == 4 || speed == 6 || speed == 8 || speed == 10 ? speed : 1;
}

// Regra histórica: preservada na retomada automática e como referência dos testes.
constexpr bool should_present(uint64_t vblank, bool resuming, unsigned speed, bool force = false) {
    return force || vblank % (resuming ? 30 : normalized(speed)) == 0;
}

// Só a seleção visual do turbo usa o relógio do host. A simulação visita todos
// os VBlanks; 1x mantém todos os quadros e a retomada conserva seu filtro antigo.
class Presentation {
public:
    bool wants(uint64_t vblank, bool resuming, unsigned requested, bool force,
               uint64_t host_ns, uint64_t period_ns) {
        if (resuming) { reset(); return should_present(vblank, true, requested, force); }
        const unsigned speed = normalized(requested);
        if (speed == 1 || !period_ns) { reset(); return true; }
        if (!initialized_ || speed_ != speed || host_ns < last_ns_ || force) {
            anchor_ns_ = last_ns_ = host_ns;
            speed_ = speed; initialized_ = true;
            return true;
        }
        last_ns_ = host_ns;
        const uint64_t elapsed = host_ns - anchor_ns_;
        if (elapsed < period_ns) return false;
        // Atraso longo começa outra janela: não desenhar de novo logo após um stall.
        // Em operação normal conserva a fase, sem construir deadline com overflow.
        if (elapsed / period_ns >= 2) anchor_ns_ = host_ns;
        else anchor_ns_ += period_ns;
        return true;
    }
    void reset() noexcept { initialized_ = false; }
private:
    uint64_t anchor_ns_ = 0, last_ns_ = 0;
    unsigned speed_ = 1;
    bool initialized_ = false;
};

class Meter {
public:
    // Os dois tempos são nanosegundos. Retorna true quando há uma amostra nova.
    bool update(uint64_t guest_ns, uint64_t host_ns) {
        if (!initialized_ || guest_ns < guest_from_ || host_ns < host_from_) {
            reset(guest_ns, host_ns);
            return false;
        }
        const auto elapsed = host_ns - host_from_;
        if (elapsed < kSampleNs) return false;
        actual_ = static_cast<double>(guest_ns - guest_from_) / static_cast<double>(elapsed);
        guest_from_ = guest_ns;
        host_from_ = host_ns;
        return true;
    }
    void reset(uint64_t guest_ns, uint64_t host_ns) {
        guest_from_ = guest_ns;
        host_from_ = host_ns;
        actual_ = 0;
        initialized_ = true;
    }
    double actual() const { return actual_; }
    static constexpr uint64_t kSampleNs = 1000000000;

private:
    uint64_t guest_from_ = 0, host_from_ = 0;
    double actual_ = 0;
    bool initialized_ = false;
};
} // namespace SotnDevSpeed
