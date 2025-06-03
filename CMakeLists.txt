cmake_minimum_required(VERSION 3.21)

project(texpack VERSION 0.2.1)

add_library(texpack
    src/texpack.cpp
    miniz/miniz.c
    spng/spng.c
)

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

target_compile_features(texpack PUBLIC cxx_std_20)

target_include_directories(texpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(texpack PUBLIC GeodeResult)

target_compile_definitions(pugixml-static PRIVATE pugi=_pugi)
target_compile_definitions(texpack PRIVATE SPNG_USE_MINIZ SPNG_STATIC)
target_include_directories(texpack PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/spng
)
target_link_libraries(texpack PRIVATE pugixml-static rectpack2D)

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
