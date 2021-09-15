cmake_minimum_required(VERSION 3.15)

set(PROJECT DoppelgangerPlugin)
project(${PROJECT})

add_subdirectory(loadPolygonMesh)
add_subdirectory(listPlugins)
add_subdirectory(getInstalledPlugins)
