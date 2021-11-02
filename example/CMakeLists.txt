add_executable(example main.cpp)
target_link_libraries(example Doppelganger)

##############################################
### app bundle for macOS (need to check on macOS...)
##############################################
if(APPLE)
    set(APP_BUNDLE MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_EXECUTABLE "example")
    set(MACOSX_BUNDLE_INFO_STRING "DoppelgangerExampleServer")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "info.n-taka")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "DoppelgangerExampleServer Version ${CMAKE_PROJECT_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_NAME "DoppelgangerExampleServer")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION})
endif()

#####################
##### Resources #####
#####################
if(WIN32)
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:example>/resources")
    add_custom_command(TARGET example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../resources" "$<TARGET_FILE_DIR:example>/resources")
elseif (APPLE)
    foreach(RES_FILE ${RESOURCES_FILES})
        file(RELATIVE_PATH RES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../resources" ${RES_FILE})
        get_filename_component(RES_DIR_PATH ${RES_PATH} DIRECTORY)
        set_property(SOURCE ${RES_FILE} PROPERTY MACOSX_PACKAGE_LOCATION "resources/${RES_DIR_PATH}")
    endforeach(RES_FILE)
endif ()
###########################
##### Resources (end) #####
###########################

