cmake_minimum_required(VERSION 3.15)

##############################################
### we use MT for users without C++ runtime
##############################################
if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()


##############################################
### at least we support macOS that is still maintained
##############################################
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "" FORCE)
endif ()


##############################################
### we use vcpkg for dependencies
##############################################
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")


##############################################
### options
##############################################
option(DOPPELGANGER_BUILD_EXAMPLE  "Build example server" ON)


##############################################
### project
##############################################
project(Doppelganger
    VERSION 2.0.0
    DESCRIPTION "HTML based interface with C++ backend"
    HOMEPAGE_URL "https://github.com/n-taka/Doppelganger"
    LANGUAGES CXX)


##############################################
### dependencies
##############################################
# Boost
if (WIN32)
    find_package(Boost REQUIRED COMPONENTS random)
elseif (APPLE)
    find_package(Boost REQUIRED COMPONENTS random filesystem)
elseif (UNIX)
    find_package(Boost REQUIRED COMPONENTS random)
endif ()
# json
find_package(nlohmann_json CONFIG REQUIRED)
# eigen3
find_package(Eigen3 CONFIG REQUIRED)
# minizip
find_package(minizip CONFIG REQUIRED)
# openSSL
find_package(OpenSSL REQUIRED)


##############################################
### target
##############################################
add_library(${PROJECT_NAME} SHARED)


##############################################
### source
##############################################
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/HTTPSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/PlainHTTPSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/SSLHTTPSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Room.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/triangleMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/WebsocketSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/PlainWebsocketSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/SSLWebsocketSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Util/download.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Util/encodeBinDataToBase64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Util/getCurrentTimestampAsString.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Util/unzip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Util/uuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Doppelganger/Util/writeBase64ToFile.cpp
)


##############################################
### include directories
##############################################
target_include_directories(${PROJECT_NAME} PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/libigl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/certify/include
)


##############################################
### libraries
##############################################
if(WIN32)
    # we need to explicitly add bcrypt.lib due to some bug.
    # we need to explicitly add Crypt32.lib due to some bug.
    # https://github.com/microsoft/vcpkg/issues/19379
    target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        Boost::random
        nlohmann_json
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
        minizip::minizip
        OpenSSL::SSL
        OpenSSL::Crypto
        Crypt32.lib
        bcrypt.lib
    )
elseif (APPLE)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(SECURITY_LIBRARY Security)
    target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        Boost::random
        Boost::filesystem
        nlohmann_json
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
        minizip::minizip
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CORE_FOUNDATION_LIBRARY}
        ${SECURITY_LIBRARY}
    )
elseif (UNIX)
    target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        Boost::random
        nlohmann_json
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
        minizip::minizip
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif ()


##############################################
### compile options
##############################################
if (WIN32)
    # because we want to use std::filesystem, we require C++17 for windows
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
    # our target is windows 10 or newer
    target_compile_definitions(${PROJECT_NAME} PUBLIC _WIN32_WINNT=0x0A00)
    # our codes heavily depend on template...
    add_definitions(/bigobj)
elseif (APPLE)
    # because we support macOS Mojave, we only require C++14 for macOS
    # we use boost::filesystem for macOS
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
elseif (UNIX)
    # because we want to use std::filesystem, we require C++17 for Linux
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif ()
target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)


##############################################
### example
##############################################
if (DOPPELGANGER_BUILD_EXAMPLE)
    add_subdirectory(example)
endif ()


##############################################
### install
##############################################
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION .)
if (WIN32)
    # *.dll file for actual execution
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION lib)
    # accompanying *.lib file for link
    install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
elseif (APPLE)
    # shared library (todo: test on macOS)
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
elseif (UNIX)
    # shared library (todo: test on Linux)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION lib)
endif ()

