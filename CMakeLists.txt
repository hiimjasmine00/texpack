cmake_minimum_required(VERSION 3.21)

project(texpack VERSION 0.4.0)

add_library(texpack
    src/texpack.cpp
    src/pugixml.cpp
)

if (NOT COMMAND CPMAddPackage)
    include(cmake/get_cpm.cmake)
endif()

if (NOT TARGET GeodeResult)
    CPMAddPackage("gh:geode-sdk/result@1.3.3")
endif()

if (NOT TARGET rectpack2D)
    CPMAddPackage("gh:TeamHypersomnia/rectpack2D#3344bf5")
endif()

if (NOT TARGET spng)
    if (NOT TARGET zlib OR NOT TARGET zlibstatic)
        CPMAddPackage(
            URI "gh:madler/zlib#5a82f71"
            GITHUB_REPOSITORY madler/zlib
            OPTIONS "ZLIB_BUILD_SHARED OFF" "ZLIB_BUILD_STATIC ON" "ZLIB_BUILD_TESTING OFF" "ZLIB_INSTALL OFF"
        )
    endif()

    CPMAddPackage(
        URI "gh:randy408/libspng@0.7.4"
        OPTIONS "BUILD_EXAMPLES OFF" "SPNG_SHARED OFF" "SPNG_STATIC ON"
        DOWNLOAD_ONLY ON
    )

    add_library(spng_static STATIC ${libspng_SOURCE_DIR}/spng/spng.c)
    target_include_directories(spng_static PUBLIC ${libspng_SOURCE_DIR}/spng)

    if (ANDROID AND ANDROID_ABI STREQUAL "armeabi-v7a")
        target_compile_definitions(spng_static PRIVATE SPNG_DISABLE_OPT)
    endif()
endif()

target_compile_features(texpack PUBLIC cxx_std_20)

target_include_directories(texpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(texpack PUBLIC GeodeResult rectpack2D spng_static zlibstatic)

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
