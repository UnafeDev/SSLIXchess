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

// local
inline constexpr uintptr_t PLAYER_HEALTH_BASE  = 0x0488FE68;
inline constexpr uintptr_t BASE_COORDS         = 0x047854A8;

inline const std::vector<unsigned int> PLAYER_HEALTH_OFFSETS { 0x18, 0x240, 0x750, 0xA0, 0x538, 0x20, 0x580 };
inline const std::vector<unsigned int> PLAYER_COORD_OFFSETS  { 0x80, 0x590, 0x240, 0x200, 0x50, 0xC0, 0x1D0 };


// enemy
inline constexpr uintptr_t E_BASE_COORDS       = 0x0445BD40;

inline const std::vector<unsigned int> E_PLAYER_COORD_OFFSETS { 0x1E0, 0x510, 0x20, 0x288, 0xA0, 0x130, 0x1D0 };




#endif // MEMOFFSETS_H
