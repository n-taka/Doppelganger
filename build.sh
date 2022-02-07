#!/bin/bash

############
# OS detection
############
TRIPLET="x"
if [ "$(uname)" == "Darwin" ]; then
    # we use custom triplet (x64-osx-mojave)
    TRIPLET="${TRIPLET}64-osx-mojave"
    # copy custom triplet file (for supporting Mojave)
    cp "${TRIPLET}.cmake" "submodule/vcpkg/triplets/${TRIPLET}.cmake"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    TRIPLET="${TRIPLET}64-windows-static"
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    TRIPLET="${TRIPLET}64-linux"
else
    echo "This OS is not supported..."
    exit 1
fi

############
# build project
############
CONFIG="Release"
if [ "$(uname)" == "Darwin" ]; then
    cmake -B build -S . -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="${CONFIG}"
    cmake --build build --config "${CONFIG}"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    subst X: .
    cd X:
    cmake -B build -S . -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="${CONFIG}"
    cmake --build build --config "${CONFIG}"
    # revert subst command
    # "/" symbol was comprehended as separator for path in MINGW. Thus, we need to explicitly use "//"
    # echo "unbind ./submodule as X:"
    subst X: //D
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    cmake -B build -S . -DVCPKG_TARGET_TRIPLET="${TRIPLET}" -DCMAKE_BUILD_TYPE="${CONFIG}"
    cmake --build build --config "${CONFIG}"
else
    echo "This OS is not supported..."
    exit 1
fi
