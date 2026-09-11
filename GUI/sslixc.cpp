//*--------------------------------------------------------------------------------*//
//* WARNING: I (UnafeDev) AM NOT RESPONSIBLE FOR ANY EFFECTS THAT COMES FROM USING *//
//*  THIS SCRIPT. THIS A CHEAT DESIGNED FOR THE GAME FPSCHESS. IS PROVIDED AS IS.  *//
//*    ANY CHANGES MADE ARE ENTIRELY UP TO THE DOWNLOADER (you) OF THE SOURCE.     *//
//*   note: if you would like to add any changes add a pull request on codeberg.   *//
//*--------------------------------------------------------------------------------*//

/*
MIT License

Copyright (c) 2026 UnafeDev

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.
*/



#include <windows.h>
#include <atomic>
#include <thread>
#include <d3d11.h>

#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "headers/threadDeclarations.h"

typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present_t oPresent = nullptr;

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dContext = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hWnd = nullptr;
bool g_ImGuiInitialized = false;

typedef BOOL(WINAPI *QueryPerformanceCounter_t)(LARGE_INTEGER*);
QueryPerformanceCounter_t oQueryPerformanceCounter = nullptr;


// Used for working the speedhack
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


// More GUI handling that I dont fully understand
// First time using ImGui lol
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

    // Handles loading buttons for the in game GUI
    // Doing ImGui::param will create a new button
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

        void CheckPlayerAuth();

        ImGui::Text("Press [F8] or [HOME] to show/hide this menu.");
        ImGui::Separator();

        ImGui::Text("Please make sure this is correct!");
        bool auth = g_IsHost.load();
        if (ImGui::Checkbox("Playing As Host", &auth))
        {
            g_IsHost.store(auth);
        };

        ImGui::Separator();

        // Where the UI begins
        // Yes, the statement order matters; wherever the button is in the code is where it is in the GUI
        bool showLoc = g_ShowEnemyLocationEnabled.load();
        if (ImGui::Checkbox("Enable Location Display", &showLoc)) {
            g_ShowEnemyLocationEnabled.store(showLoc);
        }

        if (showLoc) {
            std::lock_guard<std::mutex> lock(g_LocationMutex);
            ImGui::Text("Enemy Pos: %s", g_LocationString.c_str());
        }

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

        ImGui::Separator();
        bool fovh = g_EnableFovHack.load();
        if (ImGui::Checkbox("Enable Fov Hack", &fovh))
        {
            g_EnableFovHack.store(fovh);
        };

        float fov = g_FovValue.load();
        if (fovh)
        {
            if (ImGui::SliderFloat("Fov", &fov, 0, 250.0f, "%.1f"))
            {
                g_FovValue.store(fov);
            }
        };

        // End of GUI

        ImGui::End();
        ImGui::Render();

        g_pd3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}


// GUI input capture
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


// Loads the actual GUI
// Dont worry about having to edit this section
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

    // Handles spinning up the threads for the active cheats
    // To add more use threadDeclarations.h to create the new thread variable
    g_FreezeThread = new std::thread(FreezeWorker);
    g_AimbotThread = new std::thread(AimbotWorker);
    g_LocationThread = new std::thread(LocationWorker);
    g_FovThread = new std::thread(fovWorker);

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
    if (g_LocationThread) {
        if (g_LocationThread->joinable()) g_LocationThread->join();
        delete g_LocationThread; }

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


// Is run when injected into the game
// Do NOT remove this, change it if you want
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
