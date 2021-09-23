cmake_minimum_required(VERSION 3.15)

set(PROJECT DoppelgangerPlugin)
project(${PROJECT})

add_subdirectory(listPlugins)
add_subdirectory(updatePlugins)
add_subdirectory(shutdown)
add_subdirectory(loadPolygonMesh)
