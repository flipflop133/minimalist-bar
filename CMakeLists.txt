cmake_minimum_required(VERSION 3.21)
project(minimalist_bar C)

# Include common settings
include(build-scripts/common.cmake)

add_compile_definitions(_POSIX_C_SOURCE=200809L)

# Add source files
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

# Add libraries specific to this project
target_link_libraries(minimalist_bar PRIVATE
    pulse
    bluetooth
    cjson
    X11
    Xft
    Xrandr
    Xinerama
)
