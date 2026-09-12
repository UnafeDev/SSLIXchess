#ifndef MEMOFFSETS_H
#define MEMOFFSETS_H

#include <cstdint>
#include <vector>

// structuer that will carry everything :Sob:
struct Vector3 {
    float x;
    float y;
    float z;
};

struct Vector2 { float x, y; };

struct Matrix4x4 {
    float m[4][4];
};

extern float g_EnemyScreenX;
extern float g_EnemyScreenY;
extern bool g_DrawESP;

inline float g_EnemyScreenX = 0.0f;
inline float g_EnemyScreenY = 0.0f;
inline float g_EnemyDistanceDepth = 1.0f;
inline bool g_DrawESP = false;

// Universal
inline constexpr uintptr_t PLAYER_HEALTH_BASE    = 0x0488FE68;
inline const std::vector<unsigned int> PLAYER_HEALTH_OFFSETS    { 0x18, 0x240, 0x750, 0xA0, 0x538, 0x20, 0x580 };

// Host bound
inline constexpr uintptr_t H_FOV_BASE            = 0x043C7B38;
inline constexpr uintptr_t H_BASE_COORDS         = 0x047854A8;
inline constexpr uintptr_t H_E_BASE_COORDS       = 0x0492FE60;
inline constexpr uintptr_t H_CAMERA_MATRIX       = 0x04924CD0;

inline const std::vector<unsigned int> H_FOV_OFFSETS            { 0xF70, 0x20, 0x20, 0x258, 0x350, 0x30, 0x23C };
inline const std::vector<unsigned int> H_PLAYER_COORD_OFFSETS   { 0x80, 0x590, 0x240, 0x200, 0x50, 0xC0, 0x1D0 };
inline const std::vector<unsigned int> H_E_PLAYER_COORD_OFFSETS { 0x118, 0xA0, 0x790, 0x2E8, 0x90, 0x130, 0x1D0 };
inline const std::vector<unsigned int> H_CAMERA_MATRIX_OFFSETS  { 0x118, 0x318, 0x338, 0x4F8, 0x20, 0x548, 0x190 };

inline Vector3 g_LatestEnemyCoords = { 0.0f, 0.0f, 0.0f };

// Client Bound
inline constexpr uintptr_t C_CAMERA_MATRIX       = 0x043C7B38;
inline constexpr uintptr_t C_BASE_COORDS         = 0x0488FE78;
inline constexpr uintptr_t C_FOV_BASE            = 0x043C7AA8;
inline constexpr uintptr_t C_E_BASE_COORDS       = 0x0445B418;

inline const std::vector<unsigned int> C_COORDS_OFFSETS         { 0x0, 0x240, 0x50, 0x298, 0xE0, 0x130, 0x1D0 };
inline const std::vector<unsigned int> C_E_PLAYER_COORD_OFFSETS { 0x0, 0x530, 0xF8, 0xD0, 0x30, 0x420, 0x108 };
inline const std::vector<unsigned int> C_FOV_OFFSETS            { 0x340, 0xB0, 0x8, 0x20, 0xA8, 0x40, 0x23C };
inline const std::vector<unsigned int> C_CAMERA_MATRIX_OFFSETS  { 0x150, 0xB0, 0x20, 0xA8, 0x398, 0x548, 0x190 };

// dev note: i found out by accident that some values are host client sensitive
// i actually wanted to rip out my nerves realizing i gotta find it AGAIN
// won't give up tho </3

// TP locations
inline Vector3 SetLoc = {0.0f, 0.0f, 0.0f};

inline Vector3 g_TP_Location_BS = { 3110.95f, -8928.37f, 8857.20f };
inline Vector3 g_TP_Location_FP = { 15731.55f, 15.09f, 2457.16f };
inline Vector3 g_TP_Location_WI = { 1911.82f, 10957.10f, 2925.96f };
inline Vector3 g_TP_Location_CH = { -12.46f, 12.30f, 8605.0f };


#endif // MEMOFFSETS_H
