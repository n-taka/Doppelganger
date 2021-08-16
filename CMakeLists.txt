cmake_minimum_required(VERSION 3.15)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if(APPLE)
    # at least we support macOS that is still maintained
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "" FORCE)
endif ()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(PROJECT Doppelganger)
project(${PROJECT})
# version
set(Version "1.0")
# basically, we use Release
if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

##################
##### target #####
##################
# file(GLOB DOPPEL_CORE_CPP "include/Doppel/*.cpp")
# file(GLOB DOPPEL_UTIL_CPP "include/Doppel/util/*.cpp")
set(CPP_FILES
    main.cpp
    # ${DOPPEL_CORE_CPP}
    # ${DOPPEL_UTIL_CPP}
)

file(GLOB_RECURSE RESOURCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*")
set(RES_FILES
    ${RESOURCES_FILES}
)

# Target for this build
add_executable(${PROJECT} ${APP_BUNDLE} ${CPP_FILES} ${RES_FILES})
########################
##### target (end) #####
########################


#####################
##### Libraries #####
#####################
###
# Boost
if(WIN32)
    find_package(Boost COMPONENTS random)
elseif (APPLE)
    find_package(Boost COMPONENTS random filesystem)
endif ()

###
# json
find_package(nlohmann_json CONFIG REQUIRED)

###
# eigen3
find_package(Eigen3 3.3 REQUIRED)

# ###
# # tbb
# find_package(TBB CONFIG REQUIRED)

# ###
# # embree
# find_package(embree 3 CONFIG REQUIRED)

set(LINK_LIBS
    ${Boost_LIBRARIES}
    nlohmann_json
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
    # TBB::tbb
    # embree
)
# add OS specific libraries
if(WIN32)
    # we need to explicitly add bcrypt.lib due to some bug.
    set(LINK_LIBS
        ${LINK_LIBS}
        bcrypt.lib
    )
elseif (APPLE)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    set(LINK_LIBS
        ${LINK_LIBS}
        ${CORE_FOUNDATION_LIBRARY}
    )
endif ()

# Add external libraries
target_link_libraries(${PROJECT} PRIVATE ${LINK_LIBS})
###########################
##### Libraries (end) #####
###########################


###################
##### Include #####
###################
# Include directories for this project
set(INCLUDE_DIR
    ${EIGEN3_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/libigl/include
)

# Add a include files
target_include_directories(${PROJECT} PUBLIC ${INCLUDE_DIR})
#########################
##### Include (end) #####
#########################


###############################
##### app bundle settings #####
###############################
if(APPLE)
    set(APP_BUNDLE MACOSX_BUNDLE)
endif()

if(APPLE)
    set(MACOSX_BUNDLE_EXECUTABLE ${PROJECT})
    set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "info.n-taka")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${Version}")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${Version})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${Version})
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2020--2021.")
endif()
###############################
##### app bundle settings #####
###############################


###########################
##### compile options #####
###########################
if (WIN32)
    # because we want to use std::filesystem, we require C++17 for windows
    target_compile_features(${PROJECT} PUBLIC cxx_std_17)
elseif (APPLE)
    # because we support macOS Mojave, we only require C++14 for macOS
    # we use boost::filesystem for macOS
    target_compile_features(${PROJECT} PUBLIC cxx_std_14)
endif ()
target_compile_definitions(${PROJECT} PUBLIC _USE_MATH_DEFINES)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4819")
    add_definitions(-D_WIN32_WINNT=0x0A00)
endif ()
#################################
##### compile options (end) #####
#################################


#####################
##### Resources #####
#####################
# if(WIN32)
#     #####
#     # Resources
#     add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT}>/Resources")
#     add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Resources" "$<TARGET_FILE_DIR:${PROJECT}>/Resources")
# elseif (APPLE)
#     #####
#     # Resources
#     foreach(RES_FILE ${RESOURCES_FILES})
#         file(RELATIVE_PATH RES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Resources" ${RES_FILE})
#         get_filename_component(RES_DIR_PATH ${RES_PATH} DIRECTORY)
#         set_property(SOURCE ${RES_FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${RES_DIR_PATH}")
#     endforeach(RES_FILE)
# endif ()
###########################
##### Resources (end) #####
###########################

