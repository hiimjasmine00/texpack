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
    CPMAddPackage("gh:hiimjasmine00/libspng#1299e94")
endif()

target_compile_features(texpack PUBLIC cxx_std_20)

target_include_directories(texpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(texpack PUBLIC GeodeResult rectpack2D spng_static)

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
