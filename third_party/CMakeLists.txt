add_library(imgui
  imgui/imgui.cpp
  imgui/imgui.h
  imgui/imconfig.h
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_internal.h
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imstb_rectpack.h
  imgui/imstb_textedit.h
  imgui/imstb_truetype.h
  imgui/imfilebrowser.h
  imgui/implot.h
  imgui/implot.cpp
  imgui/implot_internal.h
  imgui/implot_items.cpp
  imgui/implot_demo.cpp
  imgui/backends/imgui_impl_sdl2.h
  imgui/backends/imgui_impl_sdl2.cpp
  imgui/backends/imgui_impl_sdlrenderer2.h
  imgui/backends/imgui_impl_sdlrenderer2.cpp)

target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/third_party/imgui ${CMAKE_SOURCE_DIR}/third_party/imgui/backends)
target_link_libraries(imgui PRIVATE SDL2::SDL2)
