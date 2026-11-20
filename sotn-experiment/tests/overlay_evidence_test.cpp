#include "sotn_overlay_evidence.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdio>
int main() {
    int reads=0;
    const auto path=std::filesystem::path("overlay.bin");
    const auto identity_path=SotnOverlayEvidence::failure_path("out",SotnOverlayEvidence::FailureReason::ProgramStartIdentityMismatch);
    const auto fatal_path=SotnOverlayEvidence::failure_path("out",SotnOverlayEvidence::FailureReason::Fatal);
    assert(identity_path.filename()=="overlay-failure-identity-mismatch.bin");
    assert(fatal_path.filename()=="overlay-failure-fatal.bin");
    assert(identity_path!=fatal_path);
    auto read=[&](uint32_t address,void* output,size_t size) {
        ++reads;
        assert(address==0x060DC000 && size==0x24000);
        auto* bytes=static_cast<uint8_t*>(output);
        for(size_t i=0;i<size;++i) bytes[i]=uint8_t(i*7);
        return true;
    };
    assert(SotnOverlayEvidence::capture(path,read) && reads==1);
    std::ifstream stream(path,std::ios::binary);
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(stream),{}};
    assert(bytes.size()==0x24000);
    for(size_t i=0;i<bytes.size();++i) assert(bytes[i]==uint8_t(i*7));
    assert(!SotnOverlayEvidence::capture("absent.bin",[](uint32_t,void*,size_t){return false;}));
    assert(!std::filesystem::exists("absent.bin"));
    assert(!SotnOverlayEvidence::capture("missing-parent/overlay.bin",read));
    std::puts("PASS: exact overlay capture, bounded range and explicit failures");
}
