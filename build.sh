#!/bin/bash

############
# OS detection
############
TRIPLET="x"
if [ "$(uname)" == "Darwin" ]; then
    # we use custom triplet (x64-osx-supported)
    # this make effect for openssl (via vcpkg)
    TRIPLET="${TRIPLET}64-osx-supported-release"
    # copy custom triplet file
    cp "${TRIPLET}.cmake" "submodule/Doppelganger_Util/submodule/vcpkg/triplets/${TRIPLET}.cmake"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    TRIPLET="${TRIPLET}64-windows-static-release"
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    TRIPLET="${TRIPLET}64-linux-release"
else
    echo "This OS is not supported..."
    exit 1
fi

############
# build project
############
if [ "$(uname)" == "Darwin" ]; then
    # Release
    cmake -B build/Release -S . -DCMAKE_TOOLCHAIN_FILE="submodule/Doppelganger_Util/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="Release"
    cmake --build build/Release --config "Release"
    # Debug
    cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE="submodule/Doppelganger_Util/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="Debug"
    cmake --build build/Debug --config "Debug"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    subst X: submodule/Doppelganger_Util/submodule
    # Release
    cmake -B build/Release -S . -DCMAKE_TOOLCHAIN_FILE="X:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="Release"
    cmake --build build/Release --config "Release"
    # Debug
    cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE="X:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="Debug"
    cmake --build build/Debug --config "Debug"

    # revert subst command
    # "/" symbol was comprehended as separator for path in MINGW. Thus, we need to explicitly use "//"
    subst X: //D
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    # Release
    cmake -B build/Release -S . -DCMAKE_TOOLCHAIN_FILE="submodule/Doppelganger_Util/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="Release"
    cmake --build build/Release --config "Release"
    # Debug
    cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE="submodule/Doppelganger_Util/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="Debug"
    cmake --build build/Debug --config "Debug"
else
    echo "This OS is not supported..."
    exit 1
fi
