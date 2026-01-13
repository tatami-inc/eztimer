cmake_minimum_required(VERSION 3.24)

project(eztimer
    VERSION 0.1.0
    DESCRIPTION "Easy timing of C++ functions"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(eztimer INTERFACE)
add_library(tatami::eztimer ALIAS eztimer)

# Switch between include directories depending on whether the downstream is
# using the build directly or is using the installed package.
include(GNUInstallDirs)
target_include_directories(eztimer
    INTERFACE 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tatami_eztimer>"
)

# Building the test-related machinery, if we are compiling this library directly.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(EMINEM_TESTS "Build eztimer's test suite." ON)
else()
    option(EMINEM_TESTS "Build eztimer's test suite." OFF)
endif()

if(EMINEM_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif() 
endif()

# Installing for find_package.
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tatami_eztimer)

install(TARGETS eztimer 
    EXPORT eztimerTargets)

install(EXPORT eztimerTargets
    FILE tatami_eztimerTargets.cmake
    NAMESPACE tatami::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tatami_eztimer)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/tatami_eztimerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tatami_eztimer)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/tatami_eztimerConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tatami_eztimerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/tatami_eztimerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tatami_eztimer)
