#include <windows.h>
#include <string>

std::wstring GetModuleDirectory(HMODULE hModule) {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(hModule, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}

DWORD WINAPI LoadActualCheatModule(LPVOID lpParam) {
    HMODULE hStub = (HMODULE)lpParam;
    Sleep(500);

    std::wstring cheatDir = GetModuleDirectory(hStub);

    // Explicitly enforce the search directory globally
    SetDllDirectoryW(cheatDir.c_str());

    // FIX: Explicitly format with a clear double backslash separator
    std::wstring dllPath = cheatDir + L"\\CheatGui.dll";

    // Load with absolute path tracking enabled
    HMODULE hRealCheat = LoadLibraryExW(dllPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (hRealCheat == nullptr) {
        // FIXED: Allocating a proper 256-byte character array buffer to hold the error text
        char errBuf[256];
        wsprintfA(errBuf, "Failed to load CheatGui.dll.\nWindows System Error Code: %lu", GetLastError());
        MessageBoxA(nullptr, errBuf, "Initialization Error", MB_ICONERROR);
    }

    return 0;
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)LoadActualCheatModule, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
