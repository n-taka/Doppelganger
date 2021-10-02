cmake_minimum_required(VERSION 3.15)

set(PROJECT DoppelgangerPlugin)
project(${PROJECT})

# debug
# add_subdirectory(configEditor)

# top menu
add_subdirectory(loadPolygonMesh)
add_subdirectory(loadCADData)
add_subdirectory(undo)
add_subdirectory(redo)
add_subdirectory(sortMeshes)
# per-mesh
add_subdirectory(loadTexture)
add_subdirectory(splitIntoIslands)
add_subdirectory(hollowMesh)
add_subdirectory(repairMesh)
add_subdirectory(saveMesh)
add_subdirectory(saveScreenshot)
add_subdirectory(removeMesh)
add_subdirectory(toggleMeshVisibility)
add_subdirectory(meshInfo)
add_subdirectory(meshErrorInfo)
# summary
add_subdirectory(allMeshesInfo)
# bottom menu
add_subdirectory(repairAllMeshes)
add_subdirectory(removeAllMeshes)
add_subdirectory(saveAllMeshes)
add_subdirectory(saveScreenshotAll)
add_subdirectory(toggleAxesVisibility)
add_subdirectory(configEditor)
add_subdirectory(listPlugins)
add_subdirectory(shutdown)
# others
add_subdirectory(updatePlugins)
add_subdirectory(pullCanvasParameters)
add_subdirectory(pushCanvasParameters)
add_subdirectory(pullCurrentMeshes)
add_subdirectory(syncCursor)
add_subdirectory(outlineOnClick)
add_subdirectory(fitToFrame)
add_subdirectory(alignCamera)
