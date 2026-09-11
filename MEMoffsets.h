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

// Universal
inline constexpr uintptr_t PLAYER_HEALTH_BASE  = 0x0488FE68;
inline const std::vector<unsigned int> PLAYER_HEALTH_OFFSETS { 0x18, 0x240, 0x750, 0xA0, 0x538, 0x20, 0x580 };

// Host bound
inline constexpr uintptr_t H_FOV_BASE            = 0x043C7B38;
inline constexpr uintptr_t H_BASE_COORDS         = 0x047854A8;
inline constexpr uintptr_t H_E_BASE_COORDS       = 0x0492FE60;

inline const std::vector<unsigned int> H_FOV_OFFSETS { 0xF70, 0x20, 0x20, 0x258, 0x350, 0x30, 0x23C };
inline const std::vector<unsigned int> H_PLAYER_COORD_OFFSETS  { 0x80, 0x590, 0x240, 0x200, 0x50, 0xC0, 0x1D0 };
inline const std::vector<unsigned int> H_E_PLAYER_COORD_OFFSETS { 0x118, 0xA0, 0x790, 0x2E8, 0x90, 0x130, 0x1D0 };

// Client Bound

inline constexpr uintptr_t C_FOV_BASE            = 0x043C7AA8;
inline constexpr uintptr_t C_E_BASE_COORDS       = 0x0445B418;

inline const std::vector<unsigned int> C_E_PLAYER_COORD_OFFSETS { 0x0, 0x530, 0xF8, 0xD0, 0x30, 0x420, 0x108 };
inline const std::vector<unsigned int> C_FOV_OFFSETS { 0x340, 0xB0, 0x8, 0x20, 0xA8, 0x40, 0x23C };

// dev note: i found out by accident that some values are host client sensitive
// i actually wanted to rip out my nerves realizing i gotta find it AGAIN
// won't give up tho </3

#endif // MEMOFFSETS_H
