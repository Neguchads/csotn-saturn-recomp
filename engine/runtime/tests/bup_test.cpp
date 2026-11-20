// The backup memory: saves written, read back by a later run, deleted, and kept where --save says.
#include "saturn.h"
#include "test.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const uint32_t kDir = 0x06010000, kData = 0x06011000, kOut = 0x06012000, kName = 0x06013000;
const uint32_t kStat = 3, kWrite = 4, kRead = 5, kDelete = 6, kDirList = 7;

// Points sat_data_dir into `root` through the platform's own variable, and returns the directory it then gives.
fs::path use_data_dir(const fs::path& root) {
#if defined(_WIN32)
    _putenv_s("APPDATA", root.string().c_str());
    return root;
#elif defined(__APPLE__)
    setenv("HOME", root.c_str(), 1);
    return root / "Library" / "Application Support";
#else
    setenv("XDG_DATA_HOME", root.c_str(), 1);
    return root;
#endif
}

fs::path scratch() {
    fs::path p = fs::temp_directory_path() / "saturn-bup-test";
    fs::remove_all(p);
    return p;
}

// A new run on a disc with this product number, saves from --save `save`.
void run(const std::string& save, const char* product = "T-1234H   ") {
    uint8_t ip[0x100] = {};
    std::memcpy(ip, "SEGA SEGASATURN ", 16);
    std::memcpy(ip + 0x20, product, 10);
    std::memcpy(ip + 0x2A, "V1.001", 6);
    g_cfg.save = save;
    bios_select_save(ip);
}

uint32_t bup(uint32_t k, uint32_t r4, uint32_t r5 = 0, uint32_t r6 = 0, uint32_t r7 = 0) {
    SH2Context& c = g_master;
    c.r[4] = r4; c.r[5] = r5; c.r[6] = r6; c.r[7] = r7;
    CHECK_EQ(bios_call(c, 0x00003000u + k * 4), 1);
    return c.r[0];
}

void name(uint32_t a, const char* s) {
    for (int i = 0; i < 12; ++i) st8(a + i, i < (int)std::strlen(s) ? s[i] : 0);
}

// Writes a save of `size` bytes, byte i being (i + seed), with r7 as `keep`.
uint32_t write(const char* save_name, uint32_t size, uint8_t seed, uint32_t keep = 0) {
    name(kDir, save_name);
    for (int i = 0; i < 11; ++i) st8(kDir + 12 + i, "a comment"[i < 9 ? i : 9]);
    st8(kDir + 23, 0);
    st32(kDir + 24, 1234);
    st32(kDir + 28, size);
    for (uint32_t i = 0; i < size; ++i) st8(kData + i, (uint8_t)(i + seed));
    return bup(kWrite, 0, kDir, kData, keep);
}

// 0 and the bytes as written, or BUP's not-found code.
uint32_t read_back(const char* save_name, uint32_t size, uint8_t seed) {
    name(kName, save_name);
    uint32_t r = bup(kRead, 0, kName, kOut);
    for (uint32_t i = 0; r == 0 && i < size; ++i) CHECK_EQ(ld8(kOut + i), (uint8_t)(i + seed));
    return r;
}

}  // namespace

// The path as argv gives it, in UTF-8; the check names the file by its characters.
TEST(a_save_under_a_non_ascii_directory_opens) {
    fs::path file = scratch() / fs::path(u8"s\u00e4ve \u2713") / "backup.bin";
    std::u8string utf8 = file.u8string();
    std::string narrow(utf8.begin(), utf8.end());
    run(narrow);
    CHECK_EQ(write("UTF8_PATH", 10, 1), 0);
    CHECK_EQ(fs::exists(file), 1);
    run(narrow);
    CHECK_EQ(read_back("UTF8_PATH", 10, 1), 0);
}

TEST(a_save_is_read_back_by_the_next_run) {
    fs::path dir = scratch(), file = dir / "nested" / "backup.bin";
    run(file.string());
    CHECK_EQ(write("BOMB_TEST", 300, 7), 0);
    CHECK_EQ(fs::exists(file), 1);
    CHECK_EQ(fs::exists(file.string() + ".new"), 0);
    run(file.string());
    CHECK_EQ(read_back("BOMB_TEST", 300, 7), 0);
    name(kName, "BOMB");
    CHECK_EQ(bup(kDirList, 0, kName, 4, 0x06015000), 1);
    CHECK_EQ(ld32(0x06015000 + 28), 300);
}

// The BIOS's own figures on Beetle Saturn, before and after Bomberman's 12-byte save.
TEST(stat_counts_free_space_as_the_bios_does) {
    run("-");
    const uint32_t stat = 0x06014000;
    CHECK_EQ(bup(kStat, 0, 0, stat), 0);
    CHECK_EQ(ld32(stat + 0), 0x8000);
    CHECK_EQ(ld32(stat + 4), 0x200);
    CHECK_EQ(ld32(stat + 8), 0x40);
    CHECK_EQ(ld32(stat + 12), 0x736E);
    CHECK_EQ(ld32(stat + 16), 0x1FE);
    CHECK_EQ(ld32(stat + 20), 0x1FE);
    CHECK_EQ(write("BOMBERSS_01", 12, 1), 0);
    CHECK_EQ(bup(kStat, 0, 0, stat), 0);
    CHECK_EQ(ld32(stat + 12), 0x7334);
    CHECK_EQ(ld32(stat + 16), 0x1FD);
    CHECK_EQ(ld32(stat + 20), 0x1FD);
    name(kName, "BOMBERSS_01");
    CHECK_EQ(bup(kDirList, 0, kName, 4, 0x06015000), 1);
    CHECK_EQ(ld16(0x06015000 + 32), 1);
}

TEST(stat_gives_no_free_bytes_on_a_full_card) {
    run("-");
    CHECK_EQ(write("FULL", 510 * 58 - 30, 1), 0);
    const uint32_t stat = 0x06014000;
    CHECK_EQ(bup(kStat, 0, 0, stat), 0);
    CHECK_EQ(ld32(stat + 12), 0);
    CHECK_EQ(ld32(stat + 16), 0);
    CHECK_EQ(ld32(stat + 20), 0);
}

TEST(saves_are_kept_by_name) {
    fs::path file = scratch() / "backup.bin";
    run(file.string());
    CHECK_EQ(write("FIRST", 10, 1), 0);
    CHECK_EQ(write("SECOND", 20, 2), 0);
    run(file.string());
    CHECK_EQ(read_back("FIRST", 10, 1), 0);
    CHECK_EQ(read_back("SECOND", 20, 2), 0);
    CHECK_EQ(read_back("THIRD", 0, 0), 5);
}

TEST(deleting_the_last_save_leaves_none) {
    fs::path file = scratch() / "backup.bin";
    run(file.string());
    CHECK_EQ(write("ONLY", 10, 1), 0);
    name(kName, "ONLY");
    CHECK_EQ(bup(kDelete, 0, kName), 0);
    run(file.string());
    CHECK_EQ(read_back("ONLY", 10, 1), 5);
}

TEST(a_run_that_saves_nothing_writes_no_file) {
    fs::path file = scratch() / "backup.bin";
    run(file.string());
    name(kName, "ANY");
    CHECK_EQ(bup(kRead, 0, kName, kOut), 5);
    bios_save();
    CHECK_EQ(fs::exists(file), 0);
}

TEST(save_dash_keeps_saves_for_the_run_alone) {
    fs::path dir = scratch();
    fs::create_directories(dir);
    use_data_dir(dir);
    run("-");
    CHECK_EQ(write("RUN_ONLY", 10, 1), 0);
    CHECK_EQ(read_back("RUN_ONLY", 10, 1), 0);
    CHECK_EQ(fs::is_empty(dir), 1);
    run("-");
    CHECK_EQ(read_back("RUN_ONLY", 10, 1), 5);
}

TEST(saves_default_to_a_folder_per_disc_in_the_data_directory) {
    fs::path dir = use_data_dir(scratch());
    run("");
    CHECK_EQ(write("DEFAULT", 10, 3), 0);
    CHECK_EQ(fs::exists(dir / "saturn-recomp" / "T-1234H_V1.001" / "backup.bin"), 1);
    run("", "T-9999H   ");
    CHECK_EQ(read_back("DEFAULT", 10, 3), 5);
    run("");
    CHECK_EQ(read_back("DEFAULT", 10, 3), 0);
}

TEST(a_damaged_file_keeps_the_saves_before_the_damage) {
    fs::path file = scratch() / "backup.bin";
    run(file.string());
    CHECK_EQ(write("GOOD", 10, 1), 0);
    CHECK_EQ(write("LATER", 4000, 2), 0);
    fs::resize_file(file, fs::file_size(file) - 100);
    run(file.string());
    CHECK_EQ(read_back("GOOD", 10, 1), 0);
    CHECK_EQ(read_back("LATER", 4000, 2), 5);
}

TEST(a_write_over_a_save_needs_r7_zero) {
    fs::path file = scratch() / "backup.bin";
    run(file.string());
    CHECK_EQ(write("SAME", 10, 1), 0);
    CHECK_EQ(write("SAME", 10, 2, 2), 6);
    CHECK_EQ(read_back("SAME", 10, 1), 0);
    CHECK_EQ(write("SAME", 10, 3), 0);
    CHECK_EQ(read_back("SAME", 10, 3), 0);
}

TEST(format_removes_all_files_and_persists_the_empty_directory) {
    fs::path file = fs::path(test_dir("sotn-bup-format")) / "backup.bin";
    run(file.string());
    CHECK_EQ(write("FIRST", 10, 1), 0);
    CHECK_EQ(write("SECOND", 20, 2), 0);
    CHECK_EQ(bup(2, 0), 0);
    name(kName, "");
    CHECK_EQ(bup(kDirList, 0, kName, 4, 0x06015000), 0);
    CHECK_EQ(bup(kStat, 0, 0, kOut), 0);
    CHECK_EQ(ld32(kOut + 16), 510);
    run(file.string());
    CHECK_EQ(read_back("FIRST", 0, 0), 5);
    CHECK_EQ(read_back("SECOND", 0, 0), 5);
    CHECK_EQ(write("NEW", 12, 3), 0);
    run(file.string());
    CHECK_EQ(read_back("NEW", 12, 3), 0);
}

TEST(nonexistent_devices_cannot_read_write_verify_delete_or_format_internal_saves) {
    run("-");
    CHECK_EQ(write("KEEP", 10, 1), 0);
    name(kName, "KEEP");
    for (uint32_t device : {1u, 2u, 0xFFFFFFFFu}) {
        CHECK_EQ(bup(1, device, 0), 1);
        CHECK_EQ(bup(2, device), 1);
        CHECK_EQ(bup(kStat, device, 0, kOut), 1);
        CHECK_EQ(bup(kWrite, device, kDir, kData, 0), 1);
        CHECK_EQ(bup(kRead, device, kName, kOut), 1);
        CHECK_EQ(bup(kDelete, device, kName), 1);
        CHECK_EQ(bup(8, device, kName, kData), 1);
        CHECK_EQ(bup(kDirList, device, kName, 4, kOut), uint32_t(-1));
        CHECK_EQ(read_back("KEEP", 10, 1), 0);
    }
    CHECK_EQ(bup(1, 0, 0), 0);
    CHECK_EQ(bup(1, 0, 1), 1);
    CHECK_EQ(read_back("KEEP", 10, 1), 0);
}

TEST(directory_reports_negative_total_when_the_table_is_too_small) {
    run("-");
    CHECK_EQ(write("FIRST", 10, 1), 0);
    CHECK_EQ(write("SECOND", 20, 2), 0);
    name(kName, "");
    st32(kOut + 36, 0x12345678);
    CHECK_EQ(bup(kDirList, 0, kName, 1, kOut), uint32_t(-2));
    CHECK_EQ(ld32(kOut + 36), 0x12345678);
    CHECK_EQ(bup(kDirList, 0, kName, 0, kOut), uint32_t(-2));
    CHECK_EQ(bup(kDirList, 0, kName, 2, kOut), 2);
}

TEST(full_backup_rejects_growth_without_losing_existing_files) {
    run("-");
    CHECK_EQ(write("FULL", 510 * 58 - 30, 1), 0);
    CHECK_EQ(write("OTHER", 1, 2), 4);
    CHECK_EQ(read_back("FULL", 1, 1), 0);
    CHECK_EQ(write("FULL", 510 * 58 - 29, 3), 4);
    CHECK_EQ(read_back("FULL", 1, 1), 0);
    CHECK_EQ(write("FULL", 10, 4), 0);
    CHECK_EQ(write("OTHER", 20, 5), 0);
    st32(kDir + 28, 0x8001u);
    CHECK_EQ(bup(kWrite, 0, kDir, kData, 0), 4);
}

TEST(failed_disk_flush_reports_failure_and_keeps_changes_for_retry) {
    fs::path root = test_dir("sotn-bup-retry"), blocker = root / "not-a-directory";
    fs::path file = blocker / "backup.bin";
    run(file.string());
    CHECK_EQ(bup(kStat, 0, 0, kOut), 0);         // carrega banco ausente antes da falha de escrita
    std::FILE* stream = std::fopen(blocker.string().c_str(), "wb");
    CHECK_EQ(stream != nullptr, 1);
    if (!stream) return;
    std::fclose(stream);
    CHECK_EQ(write("RETRY", 10, 6), 3);
    CHECK_EQ(read_back("RETRY", 10, 6), 0);
    // Só o arquivo descartável criado acima é removido para liberar o destino.
    fs::remove(blocker);
    bios_save();
    run(file.string());
    CHECK_EQ(read_back("RETRY", 10, 6), 0);
}

TEST(damaged_backup_refuses_mutations_and_preserves_the_original_file) {
    fs::path file = fs::path(test_dir("sotn-bup-damaged")) / "backup.bin";
    auto bytes = [&] {
        std::ifstream input(file, std::ios::binary);
        return std::vector<char>(std::istreambuf_iterator<char>(input), {});
    };
    run(file.string());
    CHECK_EQ(write("GOOD", 10, 1), 0);
    CHECK_EQ(write("TAIL", 4000, 2), 0);
    fs::resize_file(file, fs::file_size(file) - 100);
    const auto original = bytes();
    run(file.string());
    CHECK_EQ(read_back("GOOD", 10, 1), 0);
    CHECK_EQ(write("NEW", 10, 3), 3);
    name(kName, "GOOD");
    CHECK_EQ(bup(kDelete, 0, kName), 3);
    CHECK_EQ(bup(2, 0), 3);
    bios_save();
    CHECK_EQ(bytes() == original, true);
    // Cabeçalho incompleto também precisa proteger o arquivo, mesmo sem registro válido.
    fs::resize_file(file, 5);
    run(file.string());
    CHECK_EQ(write("NEW", 10, 3), 3);
    CHECK_EQ(fs::file_size(file), 5);
}

TEST(backup_load_limits_total_allocated_blocks_and_refuses_duplicate_records) {
    fs::path file = fs::path(test_dir("sotn-bup-capacity")) / "backup.bin";
    std::vector<char> first;
    run(file.string());
    CHECK_EQ(write("ONE", 20000, 1), 0);
    {
        std::ifstream input(file, std::ios::binary);
        first.assign(std::istreambuf_iterator<char>(input), {});
    }
    auto append = [&](bool distinct) {
        auto record = first;
        if (distinct) { record[0] = 'T'; record[1] = 'W'; record[2] = 'O'; }
        std::ofstream output(file, std::ios::binary | std::ios::app);
        output.write(record.data(), std::streamsize(record.size()));
    };
    append(true);                              // dois registros válidos, mas capacidade total inválida
    run(file.string());
    CHECK_EQ(read_back("ONE", 1, 1), 0);
    CHECK_EQ(read_back("TWO", 0, 0), 5);
    CHECK_EQ(write("NEW", 1, 2), 3);
    CHECK_EQ(fs::file_size(file), first.size() * 2);
    // Outra fixture pequena alcança a checagem de duplicata antes do limite de espaço.
    file = file.parent_path() / "duplicate.bin";
    run(file.string());
    CHECK_EQ(write("ONE", 10, 1), 0);
    {
        std::ifstream input(file, std::ios::binary);
        first.assign(std::istreambuf_iterator<char>(input), {});
    }
    append(false);
    run(file.string());
    CHECK_EQ(write("NEW", 1, 2), 3);
    CHECK_EQ(fs::file_size(file), first.size() * 2);
}

TEST(empty_save_name_is_refused_without_damaging_an_existing_bank) {
    fs::path file = fs::path(test_dir("sotn-bup-empty-name")) / "backup.bin";
    run(file.string());
    CHECK_EQ(write("KEEP", 10, 1), 0);
    auto old_size = fs::file_size(file);
    CHECK_EQ(write("", 10, 2), 8);
    CHECK_EQ(fs::file_size(file), old_size);
    run(file.string());
    CHECK_EQ(read_back("KEEP", 10, 1), 0);
    CHECK_EQ(write("VALID", 10, 3), 0);
}

TEST(an_existing_unreadable_backup_is_not_treated_as_a_new_empty_bank) {
    fs::path root = test_dir("sotn-bup-open-error"), file = root / "backup.bin";
    // Um diretório no destino produz erro de abertura sem depender de ACL/permissões.
    fs::create_directory(file);
    run(file.string());
    CHECK_EQ(write("NEW", 10, 1), 3);
    // Mesmo se o bloqueio desaparecer depois da carga, a execução não pode sobrescrever dados desconhecidos.
    fs::remove(file);
    CHECK_EQ(write("NEW", 10, 1), 3);
    bios_save();
    CHECK_EQ(fs::exists(file), false);
}

// Bomberman takes its save's comment from address 0, so the comment is the reset vector's top byte, " ".
TEST(the_bios_rom_starts_with_the_reset_vectors) {
    CHECK_EQ(bios_rom_read(0, 4), 0x20000200);
    CHECK_EQ(bios_rom_read(4, 4), 0x06002000);
    CHECK_EQ(bios_rom_read(0, 1), 0x20);
    CHECK_EQ(bios_rom_read(1, 1), 0);
    CHECK_EQ(bios_rom_read(6, 2), 0x2000);
    CHECK_EQ(bios_rom_read(8, 4), 0x20000200);
    CHECK_EQ(bios_rom_read(12, 4), 0x06002000);
    CHECK_EQ(bios_rom_read(14, 4), 0x20000000);
    CHECK_EQ(bios_rom_read(16, 4), 0);
    CHECK_EQ(bios_rom_read(0x5E4, 4), 0x000025E4);
}
