add_executable(example
    main.cpp
)

# Threads
find_package(Threads REQUIRED)

# target_link_directories(example PRIVATE "../Release")
target_link_libraries(
    example PRIVATE
    ${PROJECT_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)


##############################################
### app bundle for macOS (need to check on macOS...)
##############################################
if(APPLE)
    set_target_properties(example PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_FRAMEWORK_IDENTIFIER info.n-taka.DoppelgangerExampleServer
        MACOSX_BUNDLE_EXECUTABLE "example"
        MACOSX_BUNDLE_INFO_STRING "DoppelgangerExampleServer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "info.n-taka"
        MACOSX_BUNDLE_LONG_VERSION_STRING "DoppelgangerExampleServer Version ${CMAKE_PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_NAME "DoppelgangerExampleServer"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION}
    )
endif()

##############################################
### Resources
##############################################
if (APPLE)
    # nothing!
elseif (WIN32)
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:example>/../../Release/Doppelganger.dll" "$<TARGET_FILE_DIR:example>")
else ()
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:example>/../libDoppelganger.so" "$<TARGET_FILE_DIR:example>")
endif ()

