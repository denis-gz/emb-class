# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)

project(lesson-31-buzzer-2)

configure_file(
    "${CMAKE_SOURCE_DIR}/CMakePresets.json"
    "${CMAKE_BINARY_DIR}/CMakePresets.json"
    COPYONLY
)
