CollectSources("${PROJECT_MODULE_ROOT}/src" MOD_SOURCES)

add_library(VxGameEngine SHARED ${MOD_SOURCES})
target_include_directories(VxGameEngine PUBLIC "${PROJECT_MODULE_ROOT}/include")
target_link_libraries(VxGameEngine PUBLIC imgui SDL3::SDL3 vulkan)