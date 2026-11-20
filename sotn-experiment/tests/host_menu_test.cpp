#include "pace.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

static int require(bool condition, const char* message) {
    if (condition) return 0;
    std::fprintf(stderr, "%s\n", message);
    return 1;
}

int main(int argc, char** argv) {
    // Executável mínimo para exercitar o supervisor com processos reais, sem ROM.
    if (argc > 1) {
        std::string after;
        bool prologue = false;
        bool failure = false;
        for (int i = 1; i < argc; ++i) {
            std::printf("%s\n", argv[i]);
            if (!std::strcmp(argv[i], "--after") && i + 1 < argc) after = argv[i + 1];
            if (std::strstr(argv[i], "1800:A")) prologue = true;
            if (std::strstr(argv[i], "fatal.cue")) failure = true;
        }
        if (after.empty()) return require(false, "Supervisor must supply --after");
        if (failure) { std::fprintf(stderr, "shader failure test\n"); return 7; }
        // O primeiro processo pede título; o segundo encerra normalmente.
        if (prologue) {
            std::ofstream request(after);
            request << "title\n";
        }
        return 0;
    }
    using namespace std::chrono;
    const auto t = PaceClock::time_point{} + seconds(10);
    auto base = t;
    if (require(pace_wait(base, 16000000, t, 1) == milliseconds(16), "1x must preserve pacing")) return 1;
    base = t;
    if (require(pace_wait(base, 16000000, t, 2) == milliseconds(8), "2x must halve the host wait")) return 1;
    base = t;
    if (require(pace_wait(base, 16000000, t, 4) == milliseconds(4), "4x must quarter the host wait")) return 1;
    for (unsigned speed : {6u, 8u, 10u}) {
        base = t;
        if (require(pace_wait(base, 60000000, t, speed) == nanoseconds(60000000 / speed), "6x/8x/10x must scale the actual production pacing")) return 1;
    }
    base = t;
    if (require(pace_wait(base, 16000000, t, 0) == milliseconds(16), "Invalid factor must remain safe")) return 1;
    base = t - seconds(1);
    if (require(pace_wait(base, 16000000, t, 4) == PaceClock::duration{}, "A stalled host must not catch up")) return 1;
    if (require(base == t - milliseconds(4), "Stall reanchor must use scaled virtual time")) return 1;
    std::puts("PASS: production pacing at 1x/2x/4x/6x/8x/10x and stall recovery");
    return 0;
}
