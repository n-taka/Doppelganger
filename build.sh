#!/bin/bash

############
# OS detection
############
triplet="x"
if [ "$(uname)" == "Darwin" ]; then
    # we use custom triplet (x64-osx-mojave)
    triplet="${triplet}64-osx-mojave"
    # copy custom triplet file (for supporting Mojave)
    cp "x64-osx-mojave.cmake" "submodule/vcpkg/triplets/x64-osx-mojave.cmake"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]; then
    triplet="${triplet}86-windows"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    triplet="${triplet}64-windows-static"
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    triplet="${triplet}64-linux"
else
    echo "This OS is not supported..."
    exit 1
fi

############
# build project
############
cmake -B build -S . -DVCPKG_TARGET_TRIPLET="${triplet}"
cmake --build build --config "Release"
