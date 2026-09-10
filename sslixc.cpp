#include <windows.h>
#include <vector>
#include <atomic>
#include <thread>
#include <string>
#include <chrono>
#include <d3d11.h>

#include "MinHook.h"
#include "MEMoffsets.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present_t oPresent = nullptr;

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dContext = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hWnd = nullptr;
bool g_ImGuiInitialized = false;

std::thread* g_FreezeThread = nullptr;
std::thread* g_AimbotThread = nullptr;

std::atomic<bool> g_Running(true);
std::atomic<bool> g_AimbotEnabled(false);
std::atomic<bool> g_FreezeEnabled(false);
std::atomic<bool> g_SpeedHackEnabled(false);
std::atomic<float> g_SpeedMultiplier(1.0f);

uintptr_t baseAddress = 0;

uintptr_t calculateAddress(uintptr_t address, std::vector<unsigned int> offsets) {
    if (baseAddress == 0) return 0;
    uintptr_t addr = baseAddress + address;
    for (unsigned int offset : offsets) {
        if (addr == 0 || IsBadReadPtr((void*)addr, sizeof(uintptr_t))) return 0;
        addr = *(uintptr_t*)addr;
        addr += offset;
    }
    return addr;
}

void FreezeWorker() {
    float freezeValue = 100.0f;
    while (g_Running) {
        if (g_FreezeEnabled) {
            uintptr_t addr = calculateAddress(PLAYER_HEALTH_BASE, PLAYER_HEALTH_OFFSETS);
            if (addr != 0 && !IsBadWritePtr((void*)addr, sizeof(float))) {
                *(float*)addr = freezeValue;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AimbotWorker() {
    while (g_Running) {
        if (g_AimbotEnabled) {
            // Aimbot stuff
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

typedef BOOL(WINAPI *QueryPerformanceCounter_t)(LARGE_INTEGER*);
QueryPerformanceCounter_t oQueryPerformanceCounter = nullptr;

LARGE_INTEGER g_LastRealTime;
LARGE_INTEGER g_FakeTime;
bool g_TimeInitialized = false;

BOOL WINAPI hkQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
    LARGE_INTEGER currentRealTime;
    BOOL result = oQueryPerformanceCounter(&currentRealTime);

    if (!g_TimeInitialized) {
        g_LastRealTime = currentRealTime;
        g_FakeTime = currentRealTime;
        g_TimeInitialized = true;
    }

    LONGLONG delta = currentRealTime.QuadPart - g_LastRealTime.QuadPart;
    float currentMultiplier = g_SpeedMultiplier.load();
    LONGLONG speedHackedDelta = (LONGLONG)(delta * currentMultiplier);

    g_FakeTime.QuadPart += speedHackedDelta;
    g_LastRealTime = currentRealTime;
    lpPerformanceCount->QuadPart = g_FakeTime.QuadPart;

    return result;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC oWndProc = nullptr;

bool g_ShowMenu = true;

LRESULT WINAPI hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_ImGuiInitialized) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
            g_pd3dDevice->GetImmediateContext(&g_pd3dContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hWnd = sd.OutputWindow;

            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
            pBackBuffer->Release();

            oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

            ImGui::CreateContext();
            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);

            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            g_ImGuiInitialized = true;
        } else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    // toggle mose
    // also holds the buttons that control the cheets
    ImGuiIO& io = ImGui::GetIO();
    if (g_ShowMenu) {
        io.MouseDrawCursor = true;
    } else {
        io.MouseDrawCursor = false;
    }

    if (g_ShowMenu) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("SSLIXc FPSChess 1.2.9", &g_ShowMenu, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Press [F8] or [HOME] to show/hide this menu.");
        ImGui::Separator();

        bool aimbot = g_AimbotEnabled.load();
        if (ImGui::Checkbox("Aimbot Toggle", &aimbot)) g_AimbotEnabled.store(aimbot);

        bool freeze = g_FreezeEnabled.load();
        if (ImGui::Checkbox("Freeze Health", &freeze)) g_FreezeEnabled.store(freeze);

        bool speedh = g_SpeedHackEnabled.load();
        if (ImGui::Checkbox("Enable Speedhack", &speedh)) {
            g_SpeedHackEnabled.store(speedh);
            if (!speedh) g_SpeedMultiplier.store(1.0f);
        }

        if (speedh) {
            float speed = g_SpeedMultiplier.load();
            if (ImGui::SliderFloat("Speed Value", &speed, 1.0f, 10.0f, "%.1f")) {
                g_SpeedMultiplier.store(speed);
            }
        }

        ImGui::End();
        ImGui::Render();

        g_pd3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

LRESULT WINAPI hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_ShowMenu && g_ImGuiInitialized) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
            return true;
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI MainCheatThread(LPVOID lpParam) {
    HMODULE hModule = (HMODULE)lpParam;

    while (!GetModuleHandleA("d3d11.dll") || !GetModuleHandleA("dxgi.dll")) {
        Sleep(100);
    }
    Sleep(1000);

    baseAddress = (uintptr_t)GetModuleHandleA("FPSChess-Win64-Shipping.exe");

    if (MH_Initialize() == MH_OK) {
        MH_CreateHook(
            reinterpret_cast<LPVOID>(&QueryPerformanceCounter),
            reinterpret_cast<LPVOID>(&hkQueryPerformanceCounter),
            reinterpret_cast<LPVOID*>(&oQueryPerformanceCounter)
            );
        MH_EnableHook(reinterpret_cast<LPVOID>(&QueryPerformanceCounter));

        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX11DummyWindow", NULL };
        RegisterClassEx(&wc);
        HWND dummyHWnd = CreateWindow(wc.lpszClassName, "DX11 Dummy Window", WS_OVERLAPPEDWINDOW, 100, 100, 100, 100, NULL, NULL, wc.hInstance, NULL);

        IDXGISwapChain* pSwapChain = nullptr;
        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pContext = nullptr;

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = dummyHWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, nullptr, &pContext
            );

        if (SUCCEEDED(hr) && pSwapChain != nullptr) {
            void** pSwapChainVTable = *reinterpret_cast<void***>(pSwapChain);
            void* pPresentTarget = pSwapChainVTable[8];

            MH_CreateHook(pPresentTarget, reinterpret_cast<LPVOID>(&hkPresent), reinterpret_cast<LPVOID*>(&oPresent));
            MH_EnableHook(pPresentTarget);

            pSwapChain->Release();
            pDevice->Release();
            pContext->Release();
        }

        DestroyWindow(dummyHWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
    }

    g_FreezeThread = new std::thread(FreezeWorker);
    g_AimbotThread = new std::thread(AimbotWorker);

    while (g_Running) {
        if (GetAsyncKeyState(VK_F8) & 0x1) {
            g_ShowMenu = !g_ShowMenu;
        }

        if (GetAsyncKeyState(VK_HOME) & 0x1) {
            g_ShowMenu = !g_ShowMenu;
        }

        if (GetAsyncKeyState(VK_END) & 0x8000) {
            break;
        }

        Sleep(50);
    }

    g_Running = false;
    if (g_FreezeThread) {
        if (g_FreezeThread->joinable()) g_FreezeThread->join();
        delete g_FreezeThread;
    }
    if (g_AimbotThread) {
        if (g_AimbotThread->joinable()) g_AimbotThread->join();
        delete g_AimbotThread;
    }

    if (g_ImGuiInitialized) {
        SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    if (oPresent != nullptr) {
        MH_DisableHook(reinterpret_cast<LPVOID>(oPresent));
    }


    MH_DisableHook(reinterpret_cast<LPVOID>(&QueryPerformanceCounter));
    MH_Uninitialize();

    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, MainCheatThread, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
