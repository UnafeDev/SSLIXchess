cmake_minimum_required(VERSION 3.16)
project(SSLIXc VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(FPSChessCheat SHARED
    GUI/sslixc.cpp
    cpp/worker.cpp

    headers/MEMoffsets.h
    headers/threadDeclarations.h

    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_impl_win32.cpp
    imgui/imgui_impl_dx11.cpp
    MinSrc/buffer.c
    MinSrc/hook.c
    MinSrc/trampoline.c
    MinSrc/hde/hde32.c
    MinSrc/hde/hde64.c
    MinSrc/MinHook.h

)

target_include_directories(FPSChessCheat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/MinSrc
)

set_target_properties(FPSChessCheat PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

target_link_options(FPSChessCheat PRIVATE
    "-Wl,--dynamicbase"
    "-Wl,--nxcompat"
    "-no-pie"
    "-static"
    "-static-libgcc"
    "-static-libstdc++"
)

target_link_libraries(FPSChessCheat
    PRIVATE
        user32
        kernel32
        d3d11
        d3dcompiler
        dwmapi
)
