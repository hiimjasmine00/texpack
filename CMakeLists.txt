cmake_minimum_required(VERSION 3.21)

project(texpack VERSION 0.6.1)

add_library(texpack src/texpack.cpp)

target_compile_features(texpack PUBLIC cxx_std_20)

target_include_directories(texpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (NOT COMMAND CPMAddPackage)
    include(cmake/get_cpm.cmake)
endif()

if (TARGET fmt::fmt)
    target_link_libraries(texpack PUBLIC fmt::fmt)
elseif (TARGET fmt::fmt-header-only)
    target_link_libraries(texpack PUBLIC fmt::fmt-header-only)
else()
    CPMAddPackage("gh:fmtlib/fmt#11.1.4")
    target_link_libraries(texpack PRIVATE fmt::fmt)
endif()

if (NOT TARGET GeodeResult)
    CPMAddPackage("gh:geode-sdk/result@1.3.3")
endif()
target_link_libraries(texpack PUBLIC GeodeResult)

if (TARGET pugixml-static)
    target_link_libraries(texpack PUBLIC pugixml-static)
elseif (TARGET pugixml-shared)
    target_link_libraries(texpack PUBLIC pugixml-shared)
else()
    CPMAddPackage("gh:zeux/pugixml@1.15")
    target_link_libraries(texpack PRIVATE pugixml-static)
endif()

if (TARGET rectpack2D)
    target_link_libraries(texpack PUBLIC rectpack2D)
else()
    CPMAddPackage("gh:TeamHypersomnia/rectpack2D#3344bf5")
    target_link_libraries(texpack PRIVATE rectpack2D)
endif()

if (TARGET spng_static)
    target_link_libraries(texpack PUBLIC spng_static)
elseif (TARGET spng)
    target_link_libraries(texpack PUBLIC spng)
else()
    CPMAddPackage("gh:hiimjasmine00/libspng#73f9764")
    target_link_libraries(texpack PRIVATE spng_static)
endif()

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
