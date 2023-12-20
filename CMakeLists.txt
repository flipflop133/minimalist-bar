cmake_minimum_required(VERSION 3.10)
project(minimalist_bar)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR modules)

set(SRC_FILES
    main.c
    display.c
    i3ipc.c
    parser.c
    ${SRC_DIR}/date.c
    ${SRC_DIR}/battery.c
    ${SRC_DIR}/network.c
    ${SRC_DIR}/media.c
    ${SRC_DIR}/bluetooth.c
    ${SRC_DIR}/volume.c
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
)

# Set compiler flags for release build
target_compile_options(minimalist_bar PRIVATE -O3 -flto)

# Set compiler flags for debug build
target_compile_options(minimalist_bar PRIVATE -g -Wall -Wextra)
