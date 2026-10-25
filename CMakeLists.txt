cmake_minimum_required(VERSION 3.16)
project(rtmpbc VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Core static library
add_library(rtmpbc STATIC
    runtime-c/src/rtmpbc_fixed_arena.c
    runtime-c/src/rtmpbc_midpoint.c
    runtime-c/src/rtmpbc_multirate.c
    runtime-c/src/rtmpbc_observer.c
    runtime-c/src/rtmpbc_nonlinear.c
    runtime-c/src/rtmpbc_switched_dirac.c
    runtime-c/src/rtmpbc_fault_detector.c
)

target_include_directories(rtmpbc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime-c/include>
    $<INSTALL_INTERFACE:include>
)

if(UNIX AND NOT APPLE)
    target_link_libraries(rtmpbc PUBLIC m)
endif()

# Install rules
include(GNUInstallDirs)
install(TARGETS rtmpbc
    EXPORT rtmpbcTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY runtime-c/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT rtmpbcTargets
    FILE rtmpbcConfig.cmake
    NAMESPACE rtmpbc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rtmpbc
)

# pkg-config file generation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/rtmpbc.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/rtmpbc.pc
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rtmpbc.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# CTest Testing Suite
enable_testing()

set(TEST_SOURCES
    test_fixed_arena
    test_midpoint
    test_multirate
    test_observer
    test_nonlinear
    test_switched_dirac
    test_fault_detector
)

foreach(TEST_NAME IN LISTS TEST_SOURCES)
    add_executable(${TEST_NAME} runtime-c/tests/${TEST_NAME}.c)
    target_link_libraries(${TEST_NAME} PRIVATE rtmpbc)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
