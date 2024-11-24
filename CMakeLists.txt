cmake_minimum_required(VERSION 3.21)

project(VisionRC
    VERSION 0.1.0
    LANGUAGES CXX
)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

# 1. Look for a SDL2 package, 2. look for the SDL2 component and 3. fail if none can be found
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

# 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(SDL2_mixer REQUIRED CONFIG COMPONENTS SDL2_mixer)

set(OpenCV_STATIC ON)
set(OpenCV_SHARED OFF)
find_package(OpenCV CONFIG REQUIRED PATHS)
add_executable(VisionRC main.cpp
    app.h app.cpp
    racer.h racer.cpp
    camera.h camera.cpp
    cameraframe.h cameraframe.cpp
    race.h race.cpp
)

# SDL2::SDL2main may or may not be available. It is e.g. required by Windows GUI applications
if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(VisionRC PRIVATE SDL2::SDL2main)
endif()

if(TARGET SDL2::Mixer)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(VisionRC PRIVATE SDL2::mixer-static)
endif()

add_subdirectory(third_party) # Brings in ImGui

# Link to the actual SDL2 library. SDL2::SDL2 is the shared SDL library, SDL2::SDL2-static is the static SDL libarary.
target_link_libraries(VisionRC PRIVATE SDL2::SDL2-static SDL2_mixer::SDL2_mixer-static ${OpenCV_LIBS} imgui)

install(TARGETS VisionRC
#    RUNTIME_DEPENDENCIES
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(SOUND_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/sounds/race_checkin.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/sounds/race_ended.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/sounds/race_starting.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/sounds/tone.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/sounds/tone_high.wav
)
set(INI_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
    ${CMAKE_CURRENT_SOURCE_DIR}/default.ini
)
add_custom_command(TARGET VisionRC POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SOUND_FILES} ${INI_FILES}
    $<TARGET_FILE_DIR:VisionRC>
)
install(FILES ${SOUND_FILES} ${INI_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})

include(visionrc_cpack)
