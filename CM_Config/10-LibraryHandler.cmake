set(CURL_USE_OPENSSL ON)
set(CURL_USE_SCHANNEL OFF)

## Encryption / Decryption relevant tasks (required by CURL for HTTPS communication)
find_package(OpenSSL REQUIRED)

## fuck you too, PATCH RECENT VCPKG BULLSHIT TO MAKE CMAKE SHUT THE HELL UP
if (NOT TARGET CURL::win32_winsock)
    add_library(CURL::win32_winsock INTERFACE IMPORTED)
    set_target_properties(CURL::win32_winsock PROPERTIES
            INTERFACE_LINK_LIBRARIES "ws2_32"
    )
endif ()

## Required for Networking
find_package(CURL REQUIRED)

## Required for Local Account Storage (used by CURL)
find_package(ZLIB REQUIRED)

## Let's make it a requirement, especially for Windows users
find_package(Threads REQUIRED)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

list(APPEND APP_DEPS CURL::libcurl ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto)

if (DEFINED APP_TARGETS_ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND APP_DEPS "-static-libstdc++")
endif ()

if (APP_SYSTEM_TARGET STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32 OR MINGW)
    list(APPEND APP_DEPS crypt32 ws2_32 advapi32 secur32)

    ## issues with MinGW on Arch: re-create the libraries for OpenSSL that it needs
    # why? stupid "-ldl" flag.
    # hot garbage, am I right or am I right
    if ("dl" IN_LIST OPENSSL_LIBRARIES)
        list(REMOVE_ITEM OPENSSL_LIBRARIES "dl")
    endif ()

    set_property(TARGET OpenSSL::Crypto PROPERTY INTERFACE_LINK_LIBRARIES "crypt32;ws2_32")
    set_property(TARGET OpenSSL::SSL PROPERTY INTERFACE_LINK_LIBRARIES "crypt32;ws2_32")

    list(APPEND APP_DEPS
            "-no-pthread"
            "-Wl,--undefined=clock_gettime64"
            "-Wl,--undefined=nanosleep64"
            "-l:libwinpthread.a")

    if (APP_SYSTEM_TARGET STREQUAL "Windows")
        set(CMAKE_SHARED_LIBRARY_PREFIX "")
        set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
        set(CMAKE_STATIC_LIBRARY_PREFIX "")
        set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
    endif ()
endif ()