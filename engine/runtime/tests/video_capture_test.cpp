#include "saturn.h"
#include "state.h"
#include "test.h"
#include "video.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

TEST(graphics_capture_records_both_video_components_without_changing_them) {
    namespace fs = std::filesystem;
    const fs::path folder = test_dir("sotn-video-capture");
    State before1, before2;
    vdp1_state(before1); vdp2_state(before2);
    const auto path = folder / "graphics.bin";
    CHECK_EQ(video_capture_graphics(path.string()), true);
    std::ifstream stream(path, std::ios::binary);
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(stream), {}};
    CHECK_EQ(bytes.size(), 24 + before1.data.size() + before2.data.size());
    if (bytes.size() >= 24) {
        CHECK_EQ(std::memcmp(bytes.data(), "SOTNVDP1", 8), 0);
        uint64_t size1 = 0, size2 = 0;
        std::memcpy(&size1, bytes.data() + 8, 8); std::memcpy(&size2, bytes.data() + 16, 8);
        CHECK_EQ(size1, before1.data.size()); CHECK_EQ(size2, before2.data.size());
        if (bytes.size() == 24 + before1.data.size() + before2.data.size()) {
            CHECK_EQ(std::memcmp(bytes.data() + 24, before1.data.data(), before1.data.size()), 0);
            CHECK_EQ(std::memcmp(bytes.data() + 24 + before1.data.size(), before2.data.data(), before2.data.size()), 0);
        }
    }
    State after1, after2;
    vdp1_state(after1); vdp2_state(after2);
    CHECK_EQ(before1.data == after1.data, true); CHECK_EQ(before2.data == after2.data, true);
    CHECK_EQ(video_capture_graphics((folder / "absent/graphics.bin").string()), false);
}
