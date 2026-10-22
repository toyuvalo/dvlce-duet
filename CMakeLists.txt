cmake_minimum_required(VERSION 3.22)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" DVLCE_DUET_VERSION LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
project(DvlceDuet VERSION ${DVLCE_DUET_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(DVLCE_DUET_BUILD_PLUGIN "Build the JUCE plugin (fetches JUCE)" ON)
set(DVLCE_DUET_AAX_SDK "" CACHE PATH "AAX SDK folder (from the Avid Developer Program). Set it to build the AAX plugin for Pro Tools.")

add_library(dvlce_duet_core STATIC core/src/duet.cpp)
target_include_directories(dvlce_duet_core PUBLIC core/include)
set_target_properties(dvlce_duet_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(dvlce_duet_core PRIVATE /W4)
else()
    target_compile_options(dvlce_duet_core PRIVATE -Wall -Wextra)
endif()

add_executable(dvlce_duet_core_tests core/tests/test_duet.cpp)
target_link_libraries(dvlce_duet_core_tests PRIVATE dvlce_duet_core)

if(DVLCE_DUET_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()
