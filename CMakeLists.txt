cmake_minimum_required(VERSION 3.21)

project(texpack VERSION 0.5.2)

add_library(texpack src/texpack.cpp)

if (NOT COMMAND CPMAddPackage)
    include(cmake/get_cpm.cmake)
endif()

if (NOT TARGET fmt)
    CPMAddPackage("gh:fmtlib/fmt#11.1.4")
endif()

if (NOT TARGET GeodeResult)
    CPMAddPackage("gh:geode-sdk/result@1.3.3")
endif()

if (NOT TARGET pugixml-static)
    CPMAddPackage("gh:zeux/pugixml@1.15")
endif()

if (NOT TARGET rectpack2D)
    CPMAddPackage("gh:TeamHypersomnia/rectpack2D#3344bf5")
endif()

if (NOT TARGET spng)
    CPMAddPackage("gh:hiimjasmine00/libspng#73f9764")
endif()

target_compile_features(texpack PUBLIC cxx_std_20)

target_include_directories(texpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(texpack PUBLIC GeodeResult $<LINK_ONLY:fmt> $<LINK_ONLY:pugixml-static> $<LINK_ONLY:rectpack2D> $<LINK_ONLY:spng_static>)

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
