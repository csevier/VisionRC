cmake_minimum_required(VERSION 3.16)

project(VisionRC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Look for a SDL2 package, 2. look for the SDL2 component and 3. fail if none can be found
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

# 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(SDL2_mixer REQUIRED CONFIG COMPONENTS SDL2_mixer)

find_package(OpenCV CONFIG REQUIRED PATHS)
add_executable(VisionRC main.cpp
    app.h app.cpp
    racer.h racer.cpp
    camera.h camera.cpp
    cameraframe.h cameraframe.cpp
    race.h race.cpp)



# SDL2::SDL2main may or may not be available. It is e.g. required by Windows GUI applications
if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(VisionRC PRIVATE SDL2::SDL2main)
endif()

if(TARGET SDL2::Mixer)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(VisionRC PRIVATE SDL2::Mixer)
endif()

add_library(imgui
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imconfig.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_internal.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_rectpack.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_textedit.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_truetype.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/imfilebrowser.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdl2.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdl2.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdlrenderer2.h
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdlrenderer2.cpp)


target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/third_party/imgui ${CMAKE_SOURCE_DIR}/third_party/imgui/backends)

# Link to the actual SDL2 library. SDL2::SDL2 is the shared SDL library, SDL2::SDL2-static is the static SDL libarary.
target_link_libraries(VisionRC PRIVATE SDL2::SDL2 SDL2_mixer ${OpenCV_LIBS} imgui)

target_link_directories(VisionRC PRIVATE
    ${OpenCV_LIB_DIRS}
    )
target_include_directories(VisionRC PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    )

install(TARGETS VisionRC
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
