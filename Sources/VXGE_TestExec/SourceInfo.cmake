if (NOT DEFINED VXGE_BUILD_TEST_EXEC)
    return()
endif ()

if (NOT VXGE_BUILD_TEST_EXEC)
    return()
endif ()

find_program(GLSLC glslc REQUIRED)
CollectSources("${PROJECT_MODULE_ROOT}" MOD_SOURCES)

add_executable(VxGameTest ${MOD_SOURCES})
target_link_libraries(VxGameTest PRIVATE VxGameEngine)

add_custom_target(VxGameTestShaders ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY_ROOT}/shaders
        COMMAND ${GLSLC} ${PROJECT_MODULE_ROOT}/shaders/triangle.vert -o ${OUTPUT_DIRECTORY_ROOT}/shaders/triangle.vert.spv
        COMMAND ${GLSLC} ${PROJECT_MODULE_ROOT}/shaders/triangle.frag -o ${OUTPUT_DIRECTORY_ROOT}/shaders/triangle.frag.spv
        COMMAND ${GLSLC} ${PROJECT_MODULE_ROOT}/shaders/lit.vert -o ${OUTPUT_DIRECTORY_ROOT}/shaders/lit.vert.spv
        COMMAND ${GLSLC} ${PROJECT_MODULE_ROOT}/shaders/lit.frag -o ${OUTPUT_DIRECTORY_ROOT}/shaders/lit.frag.spv
        COMMENT "Compiling shaders"
)
add_dependencies(VxGameTest VxGameTestShaders)

file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY_ROOT}/testmodels")
file(COPY "${PROJECT_MODULE_ROOT}/testmodels/alfa_romeo_stradale_1967" DESTINATION "${OUTPUT_DIRECTORY_ROOT}/testmodels")

file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY_ROOT}/textures")
file(COPY_FILE "${PROJECT_MODULE_ROOT}/textures/checker.png" "${OUTPUT_DIRECTORY_ROOT}/textures/checker.png")