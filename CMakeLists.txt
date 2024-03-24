cmake_minimum_required(VERSION 3.10)
project(minimalist_bar)

set(CMAKE_C_STANDARD 11)

set(MODULES_DIR modules)
set(DISPLAY_DIR display)

set(SRC_FILES
    main.c
    i3ipc.c
    parser.c
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

# Set compiler flags for release build
target_compile_options(minimalist_bar PRIVATE -O3 -flto)

# Set compiler flags for debug build
target_compile_options(minimalist_bar PRIVATE -g -Wall -Wextra)
