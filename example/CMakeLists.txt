add_executable(example
    main.cpp
)

# Threads
find_package(Threads REQUIRED)

# target_link_directories(example PRIVATE "../Release")
target_link_libraries(example PRIVATE ${PROJECT_NAME} Threads::Threads)


##############################################
### app bundle for macOS (need to check on macOS...)
##############################################
if(APPLE)
    set_target_properties(example PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_FRAMEWORK_IDENTIFIER org.cmake.ExecutableTarget
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
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/css/Doppelganger.css PROPERTY MACOSX_PACKAGE_LOCATION "Resources/css")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/html/index.html PROPERTY MACOSX_PACKAGE_LOCATION "Resources/html")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon0.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon1.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon2.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon3.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon4.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon5.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon6.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon7.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon8.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/icon/cursorIcon9.png PROPERTY MACOSX_PACKAGE_LOCATION "Resources/icon")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/Canvas.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/constructMeshFrom.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/constructMeshLiFrom.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/Core.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/Modal.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/MouseKey.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/Plugin.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/request.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/Text.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/UI.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/WS.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    # set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/js/WSTasks.js PROPERTY MACOSX_PACKAGE_LOCATION "Resources/js")
    set_property(SOURCE ${CMAKE_SOURCE_DIR}/resources/defaultConfig.json PROPERTY MACOSX_PACKAGE_LOCATION "Resources")
elseif (WIN32)
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:example>/resources")
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:example>/resources")
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:example>/../../Release/Doppelganger.dll" "$<TARGET_FILE_DIR:example>")
else ()
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:example>/resources")
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:example>/resources")
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:example>/../libDoppelganger.so" "$<TARGET_FILE_DIR:example>")
endif ()

