// Inicializador próprio: não contém código, dados ou caminhos privados do jogo.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>

static std::wstring quote(const std::wstring& value) {
    std::wstring out = L"\"";
    size_t slashes = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') { ++slashes; continue; }
        out.append(ch == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        out += ch;
    }
    out.append(slashes * 2, L'\\');
    return out + L"\"";
}

int wmain(int argc, wchar_t** argv) {
    if (argc == 2 && std::wstring(argv[1]) == L"--version") {
        std::puts("CSOTN_SETUP_BOOTSTRAP_V1");
        return 0;
    }
    std::vector<wchar_t> module(32768);
    DWORD length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
    if (!length || length >= module.size()) return 2;
    std::wstring folder(module.data(), length);
    folder.resize(folder.find_last_of(L"\\/"));
    std::wstring script = folder + L"\\source\\scripts\\recomplauncher_adapter.py";
    if (GetFileAttributesW(script.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::fputs("Bootstrap source/scripts/recomplauncher_adapter.py is missing.\n", stderr);
        return 2;
    }
    std::vector<wchar_t> configured(32768);
    DWORD size = GetEnvironmentVariableW(L"SOTN_PYTHON", configured.data(), static_cast<DWORD>(configured.size()));
    if (size >= configured.size()) return 2;
    std::wstring python = size ? std::wstring(configured.data(), size) : L"py.exe";
    std::wstring command = quote(python) + (size ? L" " : L" -3 ") + quote(script);
    for (int i = 1; i < argc; ++i) command += L" " + quote(argv[i]);
    STARTUPINFOW start{};
    start.cb = sizeof(start);
    start.dwFlags = STARTF_USESTDHANDLES;
    start.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    start.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    start.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                        nullptr, folder.c_str(), &start, &process)) {
        std::fprintf(stderr, "Python 3.11+ is required. Install it or set SOTN_PYTHON. Windows error %lu\n", GetLastError());
        return 2;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return static_cast<int>(code);
}
