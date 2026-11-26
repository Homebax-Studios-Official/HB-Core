# HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
# Licensed under the Homebax Studios Non-Commercial License (see LICENSE).
cmake_minimum_required(VERSION 3.20)

project(HBCore
    VERSION 0.1.0
    DESCRIPTION "HB Core - independent web browser engine SDK by Homebax Studios"
    LANGUAGES C CXX)

if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(HBCORE_IS_TOP_LEVEL ON)
else()
    set(HBCORE_IS_TOP_LEVEL OFF)
endif()

option(HBCORE_BUILD_SHARED "Build the shared library (hbcore)" ON)
option(HBCORE_BUILD_STATIC "Build the static library (hbcore_static)" ON)
option(HBCORE_BUILD_TESTS "Build unit tests" ${HBCORE_IS_TOP_LEVEL})
option(HBCORE_BUILD_EXAMPLES "Build examples" ${HBCORE_IS_TOP_LEVEL})
option(HBCORE_BUILD_BENCHMARKS "Build benchmarks" ${HBCORE_IS_TOP_LEVEL})
option(HBCORE_INSTALL "Generate install rules" ${HBCORE_IS_TOP_LEVEL})
option(HBCORE_FETCH_GTEST "Download GoogleTest if it is not installed" ON)
option(HBCORE_TARGET_WIN7 "Windows: target Windows 7 API level (_WIN32_WINNT=0x0601). Toolchain must support it." OFF)
option(HBCORE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES AND HBCORE_IS_TOP_LEVEL)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(WIN32 AND HBCORE_TARGET_WIN7)
    add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601)
endif()

function(hbcore_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(HBCORE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(HBCORE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# hbcore_module(<name> SOURCES ... DEPS ...) -> static library hb_<name>, independently testable.
function(hbcore_module name)
    cmake_parse_arguments(M "" "" "SOURCES;DEPS" ${ARGN})
    add_library(hb_${name} STATIC ${M_SOURCES})
    target_include_directories(hb_${name} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    target_compile_features(hb_${name} PUBLIC cxx_std_20)
    if(M_DEPS)
        target_link_libraries(hb_${name} PUBLIC ${M_DEPS})
    endif()
    hbcore_warnings(hb_${name})
endfunction()

find_package(Threads REQUIRED)

hbcore_module(base SOURCES
    core/base/arena.cpp core/base/log.cpp core/base/result.cpp core/base/scheduler.cpp core/base/strings.cpp
    DEPS Threads::Threads)
hbcore_module(dom SOURCES core/dom/node.cpp DEPS hb_base)
hbcore_module(html SOURCES core/html/tokenizer.cpp core/html/parser.cpp DEPS hb_dom)
hbcore_module(css SOURCES core/css/color.cpp core/css/stylesheet.cpp core/css/style.cpp core/css/ua_stylesheet.cpp DEPS hb_dom)
hbcore_module(graphics SOURCES core/graphics/bitmap_font.cpp core/graphics/surface.cpp DEPS hb_css)
hbcore_module(layout SOURCES core/layout/layout.cpp DEPS hb_css hb_graphics)
hbcore_module(paint SOURCES core/paint/painter.cpp DEPS hb_layout hb_graphics)
hbcore_module(engine SOURCES core/engine/page.cpp DEPS hb_html hb_paint)

set(HBCORE_API_SOURCES core/engine/hb_core.cpp bindings/c/hb_core_c.cpp)

if(HBCORE_BUILD_SHARED)
    add_library(hbcore SHARED ${HBCORE_API_SOURCES})
    add_library(HB::Core ALIAS hbcore)
    target_link_libraries(hbcore PRIVATE hb_engine)
    target_compile_definitions(hbcore PRIVATE HBCORE_BUILDING)
    target_include_directories(hbcore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_compile_features(hbcore PUBLIC cxx_std_20)
    set_target_properties(hbcore PROPERTIES
        VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME hbcore EXPORT_NAME Core)
    hbcore_warnings(hbcore)
endif()

if(HBCORE_BUILD_STATIC)
    add_library(hbcore_static STATIC ${HBCORE_API_SOURCES})
    add_library(HB::CoreStatic ALIAS hbcore_static)
    target_link_libraries(hbcore_static PUBLIC hb_engine)
    target_compile_definitions(hbcore_static PUBLIC HBCORE_STATIC)
    target_include_directories(hbcore_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    set_target_properties(hbcore_static PROPERTIES OUTPUT_NAME hbcore_static)
    hbcore_warnings(hbcore_static)
endif()

if(HBCORE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(HBCORE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
if(HBCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(HBCORE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(_hb_install_targets)
    if(HBCORE_BUILD_SHARED)
        list(APPEND _hb_install_targets hbcore)
    endif()
    install(DIRECTORY include/hbcore DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    if(_hb_install_targets)
        install(TARGETS ${_hb_install_targets} EXPORT HBCoreTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
        install(EXPORT HBCoreTargets NAMESPACE HB:: FILE HBCoreTargets.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HBCore)
        configure_package_config_file(cmake/HBCoreConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/HBCoreConfig.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HBCore)
        write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/HBCoreConfigVersion.cmake
            VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/HBCoreConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/HBCoreConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HBCore)
    endif()
    install(FILES LICENSE COPYRIGHT.md THIRD_PARTY_NOTICES.md README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
