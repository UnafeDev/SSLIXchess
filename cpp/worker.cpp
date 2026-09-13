#include <cmath>
#include <windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <d3d11.h>

#include "headers/threadDeclarations.h"
#include "headers/MEMoffsets.h"

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
            snprintf(buffer, sizeof(buffer), "(%.1f, %.1f, %.1f)", enemyCoords.x, enemyCoords.y, enemyCoords.z);

            {
                std::lock_guard<std::mutex> lock(g_LocationMutex);
                g_LocationString = buffer;
                g_LatestEnemyCoords = enemyCoords;
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

void HostTP()
{
    while (baseAddress == 0 && g_Running)
    {
        Sleep(100);
    }
    while (g_Running)
    {
        if (g_TP_Pressed)
        {
            uintptr_t currentLocationAddr = 0;

            if (!g_TP_Enemy){currentLocationAddr = calculateAddress(H_BASE_COORDS, H_PLAYER_COORD_OFFSETS);}
            else {currentLocationAddr = calculateAddress(H_E_BASE_COORDS, H_E_PLAYER_COORD_OFFSETS);} // not working

            if (currentLocationAddr != 0 && g_IsHost)
            {
                *(Vector3*)currentLocationAddr = SetLoc;
            };
        }

        Sleep(10);
    }
}


// all of this is for esp
// WTS is to let ImGui paint a box in the correct area
// Basically 3D -> 2D mixed space for the box to render properly
bool WorldToScreen(Vector3 worldPos, Vector2& screenPos, Matrix4x4 matrix, int windowWidth, int windowHeight, float& outDepth) {
    Vector3 camPos = { matrix.m[3][0], matrix.m[3][1], matrix.m[3][2] };

    Vector3 relativeVector = { worldPos.x - camPos.x, worldPos.y - camPos.y, worldPos.z - camPos.z };

    Vector3 vForward = { matrix.m[0][0], matrix.m[1][0], matrix.m[2][0] };
    Vector3 vRight   = { matrix.m[0][1], matrix.m[1][1], matrix.m[2][1] };
    Vector3 vUp      = { matrix.m[0][2], matrix.m[1][2], matrix.m[2][2] };

    float wDepth = (relativeVector.x * vForward.x) + (relativeVector.y * vForward.y) + (relativeVector.z * vForward.z);

    if (wDepth < 0.1f) return false;
    outDepth = wDepth;

    float xProjected = (relativeVector.x * vRight.x) + (relativeVector.y * vRight.y) + (relativeVector.z * vRight.z);
    float yProjected = (relativeVector.x * vUp.x)    + (relativeVector.y * vUp.y)    + (relativeVector.z * vUp.z);

    float screenCenterX = windowWidth / 2.0f;
    float screenCenterY = windowHeight / 2.0f;

    screenPos.x = screenCenterX + (xProjected * (screenCenterX / wDepth));
    screenPos.y = screenCenterY - (yProjected * (screenCenterY / wDepth));

    return true;
}

struct CameraProperties {
    Vector3 Position;
    Vector3 Rotation;
};

inline float DegreesToRadians(float degrees) {
    return degrees * (3.1415926535f / 180.0f);
}

// Enemy location grabber for the 2D space
// Also calculates the FOV, yaw, pitch, and roll for accurate box sizes
bool CalculateScreenCoordinates(Vector3 targetPos, CameraProperties cam, float currentFov, int width, int height, Vector2& screenPos) {
    Vector3 delta = { targetPos.x - cam.Position.x, targetPos.y - cam.Position.y, targetPos.z - cam.Position.z };

    float radPitch = DegreesToRadians(cam.Rotation.x);
    float radYaw   = DegreesToRadians(cam.Rotation.y);

    float cosYaw = cosf(radYaw);
    float sinYaw = sinf(radYaw);
    float cosPitch = cosf(radPitch);
    float sinPitch = sinf(radPitch);

    float forwardDepth = delta.x * (cosPitch * cosYaw) + delta.y * (cosPitch * sinYaw) + delta.z * sinPitch;
    if (forwardDepth < 1.0f) return false;

    float rightAxis = delta.x * (-sinYaw) + delta.y * cosYaw;
    float upAxis    = delta.x * (-sinPitch * cosYaw) + delta.y * (-sinPitch * sinYaw) + delta.z * cosPitch;

    float safeFov = (currentFov > 10.0f && currentFov < 180.0f) ? currentFov : 90.0f;
    float fovScaling = 1.0f / tanf(DegreesToRadians(safeFov / 2.0f));

    float screenWidth = (width > 0) ? (float)width : 1920.0f;
    float screenHeight = (height > 0) ? (float)height : 1080.0f;
    float aspectRatio = screenWidth / screenHeight;

    float driftCalibrationModifier = 1.75f;
    float driftCalibrationModifierY = 1.65f;

    float deviceX = (rightAxis * (fovScaling * driftCalibrationModifier)) / (forwardDepth * aspectRatio);
    float deviceY = (upAxis * fovScaling * driftCalibrationModifierY) / forwardDepth;

    screenPos.x = (screenWidth / 2.0f) + (deviceX * (screenWidth / 2.0f));
    screenPos.y = (screenHeight / 2.0f) - (deviceY * (screenHeight / 2.0f));

    return true;
}

// reads, writes and enables the ESP box to be drawn
void espWorker()
{
    while (baseAddress == 0 && g_Running)
    {
        Sleep(100);
    }

    while (g_Running)
    {
        uintptr_t rotationAddr = 0;
        uintptr_t fovAddr = 0;
        uintptr_t myCoordsAddr = 0;
        uintptr_t enemyCoordsAddr = 0;

        if (g_IsHost) {
            rotationAddr    = calculateAddress(H_CAMERA_MATRIX, H_CAMERA_MATRIX_OFFSETS);
            fovAddr        = calculateAddress(H_FOV_BASE, H_FOV_OFFSETS);
            myCoordsAddr   = calculateAddress(H_BASE_COORDS, H_PLAYER_COORD_OFFSETS);
            enemyCoordsAddr= calculateAddress(H_E_BASE_COORDS, H_E_PLAYER_COORD_OFFSETS);
        } else {
            rotationAddr    = calculateAddress(C_CAMERA_MATRIX, C_CAMERA_MATRIX_OFFSETS);
            fovAddr        = calculateAddress(C_FOV_BASE, C_FOV_OFFSETS);
            myCoordsAddr   = calculateAddress(C_BASE_COORDS, C_COORDS_OFFSETS);
            enemyCoordsAddr= calculateAddress(C_E_BASE_COORDS, C_E_PLAYER_COORD_OFFSETS);
        }

        if (rotationAddr != 0 && fovAddr != 0 && myCoordsAddr != 0 && enemyCoordsAddr != 0 &&
            !IsBadReadPtr((void*)rotationAddr, sizeof(Vector3)) &&
            !IsBadReadPtr((void*)fovAddr, sizeof(float)) &&
            !IsBadReadPtr((void*)myCoordsAddr, sizeof(Vector3)) &&
            !IsBadReadPtr((void*)enemyCoordsAddr, sizeof(Vector3)))
        {
            Vector3 localCamRot = *reinterpret_cast<Vector3*>(rotationAddr);
            float localFov = *reinterpret_cast<float*>(fovAddr);
            Vector3 localCamPos = *reinterpret_cast<Vector3*>(myCoordsAddr);
            Vector3 enemyCoordsTarget = *reinterpret_cast<Vector3*>(enemyCoordsAddr);

            CameraProperties localCam;
            localCam.Position = localCamPos;
            localCam.Rotation = localCamRot;

            Vector2 screenCoords;
            int screenWidth = 1920;
            int screenHeight = 1080;

            if (CalculateScreenCoordinates(enemyCoordsTarget, localCam, localFov, screenWidth, screenHeight, screenCoords))
            {
                g_EnemyScreenX.store(screenCoords.x);
                g_EnemyScreenY.store(screenCoords.y);

                float dx = enemyCoordsTarget.x - localCam.Position.x;
                float dy = enemyCoordsTarget.y - localCam.Position.y;
                float dz = enemyCoordsTarget.z - localCam.Position.z;
                g_EnemyDistanceDepth = sqrtf(dx * dx + dy * dy + dz * dz);

                g_DrawESP.store(true);
            }
            else
            {
                g_DrawESP.store(false);
            }
        }
        else
        {
            g_DrawESP.store(false);
        }

        Sleep(16);
    }
}

// Do I really need a comment for this?
// Don't work yet

void AimbotWorker() {
    float remainderX = 0.0f;
    float remainderY = 0.0f;

    while (g_Running) {
        if (g_AimbotEnabled.load() && g_DrawESP.load()) {
            float screenCenterX = 1920.0f / 2.0f;
            float screenCenterY = 1080.0f / 2.0f;

            float targetX = g_EnemyScreenX.load();
            float targetY = g_EnemyScreenY.load();

            float x = (targetX - screenCenterX) + remainderX;
            float y = (targetY - screenCenterY) + remainderY;

            float distanceFromCenter = sqrtf(x * x + y * y);
            float maxFovPixels = 300.0f;

            if (distanceFromCenter < maxFovPixels && distanceFromCenter > 1.0f) {
                float smoothing = 5.0f;

                float moveXFloat = x / smoothing;
                float moveYFloat = y / smoothing;

                LONG moveX = (LONG)moveXFloat;
                LONG moveY = (LONG)moveYFloat;

                // Save fractional remainder for the next tick
                remainderX = moveXFloat - (float)moveX;
                remainderY = moveYFloat - (float)moveY;

                if (moveX != 0 || moveY != 0) {
                    INPUT input = { 0 };
                    input.type = INPUT_MOUSE;
                    input.mi.dx = moveX;
                    input.mi.dy = moveY;
                    input.mi.dwFlags = MOUSEEVENTF_MOVE;
                    SendInput(1, &input, sizeof(INPUT));
                }
            } else {
                remainderX = 0.0f;
                remainderY = 0.0f;
            }
        } else {
            remainderX = 0.0f;
            remainderY = 0.0f;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}