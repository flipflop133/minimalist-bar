cmake_minimum_required(VERSION 3.21)
project(minimalist_bar C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_LINKER /usr/bin/ld.lld)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(MODULES_DIR src/modules)
set(DISPLAY_DIR src/display)

set(SRC_FILES
    src/main.c
    src/i3ipc.c
    src/parser.c
    ${MODULES_DIR}/date.c
    ${MODULES_DIR}/battery.c
    ${MODULES_DIR}/network.c
    ${MODULES_DIR}/media.c
    ${MODULES_DIR}/bluetooth.c
    ${MODULES_DIR}/volume.c
    ${DISPLAY_DIR}/display.c
    ${DISPLAY_DIR}/modules.c
    ${DISPLAY_DIR}/workspaces.c
)

add_executable(minimalist_bar ${SRC_FILES})

# Add include directories
target_include_directories(minimalist_bar PRIVATE
    /usr/include
    /usr/include/freetype2/
)

# Add libraries
target_link_libraries(minimalist_bar PRIVATE
    pulse
    bluetooth
    cjson
    X11
    Xft
    Xrandr
    Xinerama
)

target_compile_definitions(minimalist_bar PRIVATE _POSIX_C_SOURCE=200809L)


# Set compiler flags for release build
target_compile_options(minimalist_bar PRIVATE -O3 -flto)

# Set compiler flags for debug build
target_compile_options(minimalist_bar PRIVATE -g -Wall -Wextra)

# message compiler
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler Version: ${CMAKE_C_COMPILER_VERSION}")

# message linker
message(STATUS "Linker: ${CMAKE_LINKER}")
message(STATUS "Linker Version: ${CMAKE_LINKER_VERSION}")