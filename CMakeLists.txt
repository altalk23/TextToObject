cmake_minimum_required(VERSION 3.3.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TextObject VERSION 1.0.0)

include(cmake/CPM.cmake)

file(GLOB SOURCES
	src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(PkgConfig REQUIRED)
pkg_search_module(FFTW REQUIRED fftw3 IMPORTED_TARGET)

CPMAddPackage("gh:SFML/SFML#2.5.1")
CPMAddPackage("gh:oneapi-src/oneTBB@2021.9.0")


target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_link_libraries(${PROJECT_NAME}
    PkgConfig::FFTW
    sfml-graphics
    TBB::tbb
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

add_executable(testing
    src/ExecMain.cpp
    src/MatrixOperations.cpp
)

target_link_libraries(testing
    PkgConfig::FFTW
    sfml-graphics
)

target_include_directories(testing PUBLIC
    include
)