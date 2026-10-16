cmake_minimum_required(VERSION 3.27)

project(hyprlustre
    DESCRIPTION "Hyprlustre visual effects for Hyprland"
    VERSION 0.1.0
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

add_library(lustre SHARED
    src/main.cpp
    src/LustreConfig.cpp
    src/LustreDecoration.cpp
)

target_link_libraries(lustre PRIVATE rt PkgConfig::deps)
