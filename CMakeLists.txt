cmake_minimum_required(VERSION 3.21)

project(texpack)

add_library(texpack INTERFACE)

if (NOT COMMAND CPMAddPackage)
    include(cmake/get_cpm.cmake)
endif()

if (NOT TARGET GeodeResult)
    CPMAddPackage("gh:geode-sdk/result@1.3.3")
endif()

if (NOT TARGET pugixml AND NOT TARGET pugixml-static)
    CPMAddPackage("gh:zeux/pugixml@1.15")
endif()

if (NOT TARGET rectpack2D)
    CPMAddPackage("gh:TeamHypersomnia/rectpack2D#3344bf5")
endif()

set_target_properties(texpack PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(texpack INTERFACE SPNG_USE_MINIZ SPNG_STATIC)
target_compile_features(texpack INTERFACE cxx_std_20)

target_sources(texpack INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/texpack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/miniz/miniz.c
    ${CMAKE_CURRENT_SOURCE_DIR}/spng/spng.c
)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/texpack.cpp PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
)

target_include_directories(texpack INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/spng
)
target_link_libraries(texpack INTERFACE GeodeResult pugixml-static rectpack2D)

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
