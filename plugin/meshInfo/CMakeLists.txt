cmake_minimum_required(VERSION 3.15)

set(PROJECT meshInfo)
project(${PROJECT})

add_custom_target(${PROJECT} ALL)

#####################
##### Resources #####
#####################
# directory
add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${PROJECT}")
# dll
# add_custom_command(TARGET ${PROJECT} COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT}>" "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${PROJECT}/")
# module.js
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/module.js")
    add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/module.js" "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${PROJECT}/")
endif()

# create zip archive
# https://stackoverflow.com/questions/44796465/cmake-zip-folder-but-remove-leading-path
add_custom_command(
    TARGET ${PROJECT}
    COMMAND ${CMAKE_COMMAND} POST_BUILD -E tar "cfv" "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${PROJECT}.zip" --format=zip -- .
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${PROJECT}/")
###########################
##### Resources (end) #####
###########################

