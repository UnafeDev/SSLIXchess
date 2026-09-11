#ifndef THREADDECLARATIONS_H
#define THREADDECLARATIONS_H
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

// This header holds all of the variable declarations for threads and functions.
// It should be included in all cpp files for functionality
// to find the actual cheat functions, make your way to worker.cpp

inline std::atomic<bool> g_IsHost(false);

void FreezeWorker();
void AimbotWorker();
void LocationWorker();
void fovWorker();

void CheckPlayerAuth();

inline uintptr_t baseAddress = 0;

inline std::thread* g_FreezeThread = nullptr;
inline std::thread* g_AimbotThread = nullptr;
inline std::thread* g_LocationThread = nullptr;
inline std::thread* g_FovThread = nullptr;

inline std::atomic<bool> g_Running(true);
inline std::atomic<bool> g_AimbotEnabled(false);
inline std::atomic<bool> g_FreezeEnabled(false);
inline std::atomic<bool> g_SpeedHackEnabled(false);
inline std::atomic<bool> g_ShowEnemyLocationEnabled(false);
inline std::atomic<bool> g_EnableFovHack(false);

inline std::atomic<float> g_SpeedMultiplier(1.0f);
inline std::atomic<float> g_FovValue(90.0f);

inline std::string g_LocationString = "Waiting...";
inline std::mutex g_LocationMutex;

#endif // THREADDECLARATIONS_H
