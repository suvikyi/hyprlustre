cmake_minimum_required(VERSION 3.27)

project(hyprion
    DESCRIPTION "Hyprion visual effects for Hyprland"
    VERSION 0.2.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    libdrm
    libinput
    libudev
    pangocairo
    pixman-1
    wayland-server
    xkbcommon
)

add_library(hyprion SHARED
    src/main.cpp
    src/HyprionConfig.cpp
    src/HyprionDecoration.cpp
)

target_link_libraries(hyprion PRIVATE rt PkgConfig::deps)
