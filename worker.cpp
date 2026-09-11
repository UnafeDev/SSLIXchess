#include <windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <d3d11.h>
#include <string.h>

#include "threadDeclarations.h"
#include "MEMoffsets.h"


// Address helper
// Fetches the right address from the BASE and OFFSETS
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


// Freezes the player's health at 100
// Only works as the host sadly
// If I can spoof RPCs I'll update it
void FreezeWorker() {
    float freezeValue = 100.0f;
    while (g_Running) {
        if (g_FreezeEnabled) {
            uintptr_t addr = calculateAddress(PLAYER_HEALTH_BASE, PLAYER_HEALTH_OFFSETS); // no separate if check because health addrs are universal
            if (addr != 0 && !IsBadWritePtr((void*)addr, sizeof(float))) {
                *(float*)addr = freezeValue;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Do I really need a comment for this?
// Don't work yet
void AimbotWorker() {
    while (g_Running) {
        if (g_AimbotEnabled.load()) {
            // Aimbot stuff
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Catches the enemy location and displays it as text
// Will potentially be used for ESP
void LocationWorker() {
    while (baseAddress == 0 && g_Running) {
        Sleep(100);
    }

    while (g_Running) {
        if (!g_ShowEnemyLocationEnabled.load()) {
            Sleep(100);
            continue;
        }

        uintptr_t enemyCoordsAddr = 0;
        if (g_IsHost){enemyCoordsAddr = calculateAddress(H_E_BASE_COORDS, H_E_PLAYER_COORD_OFFSETS);}
        else{enemyCoordsAddr = calculateAddress(C_E_BASE_COORDS, C_E_PLAYER_COORD_OFFSETS);};

        if (enemyCoordsAddr != 0)
        {
            Vector3 enemyCoords{ 0.0f, 0.0f, 0.0f };

            if (enemyCoordsAddr && !IsBadReadPtr(reinterpret_cast<const void*>(enemyCoordsAddr), sizeof(Vector3))) {
                enemyCoords = *reinterpret_cast<Vector3*>(enemyCoordsAddr);
            }

            char buffer[256];
            snprintf(buffer, sizeof(buffer), "(%.1f, %.1f, %.1f)",
                     enemyCoords.x, enemyCoords.y, enemyCoords.z);
            {
                std::lock_guard<std::mutex> lock(g_LocationMutex);
                g_LocationString = buffer;
            }
        };
        Sleep(50);
    }
}


// Fov changing function
void fovWorker(){
    while (baseAddress == 0 && g_Running)
    {
        Sleep(100);
    };

    while (g_Running)
    {
        uintptr_t fovAddr = 0;
        if (!g_IsHost) {fovAddr = calculateAddress(C_FOV_BASE, C_FOV_OFFSETS);} // idk why i did this one as a !, im stupid
        else{fovAddr = calculateAddress(H_FOV_BASE, H_FOV_OFFSETS);};

        if (fovAddr != 0)
        {
            float fov = g_FovValue.load();

            if (fovAddr != 0 && !IsBadWritePtr((void*)fovAddr, sizeof(float))) {
                *(float*)fovAddr = fov;
            }
        };
        Sleep(10);
    }
}

void CheckPlayerAuth()
{
    // ruahasdash
};