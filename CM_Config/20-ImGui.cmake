add_library(imgui SHARED
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/backends/imgui_impl_sdl3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/backends/imgui_impl_sdlrenderer3.cpp"
)

target_include_directories(imgui PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/backends")

target_link_libraries(imgui PUBLIC SDL3::SDL3)