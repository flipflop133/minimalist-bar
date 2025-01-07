#!/bin/bash

BUILD_DIR="build"
BUILD_TYPE="Release" # Default build type

# Check if the build type argument is provided
if [ "$#" -gt 0 ]; then
    BUILD_TYPE="$1"
fi

# Check if the build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating $BUILD_DIR directory..."
    mkdir "$BUILD_DIR" || exit 1
fi

cd "$BUILD_DIR" || exit 1

echo "Build type: $BUILD_TYPE"

# Set the compiler to Clang
export CC=clang
export CXX=clang++

# Run CMake with the specified build type
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# Build the project
make
