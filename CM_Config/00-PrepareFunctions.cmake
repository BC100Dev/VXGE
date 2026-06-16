function(CollectSources RootDir OutVar)
    set(SEARCH_PATTERNS
            "${RootDir}/*.cpp"
            "${RootDir}/*.hpp"
            "${RootDir}/*.c"
            "${RootDir}/*.h"
            "${RootDir}/*.cc"
            "${RootDir}/*.cuh"
            "${RootDir}/**/*.cpp"
            "${RootDir}/**/*.hpp"
            "${RootDir}/**/*.c"
            "${RootDir}/**/*.h"
            "${RootDir}/**/*.cc"
            "${RootDir}/**/*.cuh")

    file(GLOB_RECURSE SOURCE_FILES ${SEARCH_PATTERNS})
    set(${OutVar} ${SOURCE_FILES} PARENT_SCOPE)
endfunction()

function(TestCLibraryFunctionality C_LIB_NAME SRC OUT_VAR)
    # write and check
    file(WRITE "${CMAKE_BINARY_DIR}/${C_LIB_NAME}_check.c" "${SRC}")
    try_compile(RESULT_VAR "${CMAKE_BINARY_DIR}/${C_LIB_NAME}_check" "${CMAKE_BINARY_DIR}/${C_LIB_NAME}_check.c")
    set(${OUT_VAR} ${RESULT_VAR} PARENT_SCOPE)

    # delete temporary files
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/${C_LIB_NAME}_check")
    file(REMOVE "${CMAKE_BINARY_DIR}/${C_LIB_NAME}_check.c")
endfunction()