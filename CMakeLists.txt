cmake_minimum_required(VERSION 3.13)
project(simSDF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
endif()

find_package(Boost COMPONENTS filesystem REQUIRED)

set(USE_SYSTEM_GZLIBS OFF CACHE BOOL "Use system gz libs (gz-math, sdformat, etc...) instead of building them")
if(USE_SYSTEM_GZLIBS)
    find_package(gz-math7 REQUIRED)
    find_package(sdformat13 REQUIRED)
else()
    include(ExternalProject)
    set(GZLIBS_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gzlibs)
    if(CMAKE_TOOLCHAIN_FILE)
        set(EP_COMMON_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}")
    endif()
    ExternalProject_Add(ext-gz-cmake
        URL https://github.com/gazebosim/gz-cmake/archive/refs/tags/gz-cmake3_3.0.0.tar.gz
        CMAKE_ARGS
            ${EP_COMMON_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${GZLIBS_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH=${GZLIBS_INSTALL_PREFIX}
            -DBUILD_TESTING=OFF
            -DBUILD_DOCS=OFF
    )
    ExternalProject_Add(ext-gz-tools
        DEPENDS ext-gz-cmake
        URL https://github.com/gazebosim/gz-tools/archive/refs/tags/gz-tools2_2.0.0.tar.gz
        CMAKE_ARGS
            ${EP_COMMON_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${GZLIBS_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH=${GZLIBS_INSTALL_PREFIX}
            -DBUILD_TESTING=OFF
            -DBUILD_DOCS=OFF
    )
    ExternalProject_Add(ext-gz-utils
        DEPENDS ext-gz-tools
        URL https://github.com/gazebosim/gz-utils/archive/refs/tags/gz-utils2_2.0.0.tar.gz
        CMAKE_ARGS
            ${EP_COMMON_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${GZLIBS_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH=${GZLIBS_INSTALL_PREFIX}
            -DBUILD_TESTING=OFF
            -DBUILD_DOCS=OFF
    )
    ExternalProject_Add(ext-gz-math
        DEPENDS ext-gz-utils
        URL https://github.com/gazebosim/gz-math/archive/refs/tags/gz-math7_7.0.2.tar.gz
        CMAKE_ARGS
            ${EP_COMMON_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${GZLIBS_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH=${GZLIBS_INSTALL_PREFIX}
            -DBUILD_TESTING=OFF
            -DBUILD_DOCS=OFF
            -DSKIP_PYBIND11=ON
    )
    ExternalProject_Add(ext-sdformat
        DEPENDS ext-gz-math
        URL https://github.com/gazebosim/sdformat/archive/refs/tags/sdformat13_13.0.1.tar.gz
        CMAKE_ARGS
            ${EP_COMMON_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${GZLIBS_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH=${GZLIBS_INSTALL_PREFIX}
            -DBUILD_TESTING=OFF
            -DSKIP_PYBIND11=ON
    )
endif()

if(NOT COPPELIASIM_INCLUDE_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(COPPELIASIM_INCLUDE_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/include)
    else()
        set(COPPELIASIM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${COPPELIASIM_INCLUDE_DIR}/cmake)
find_package(CoppeliaSim 4.5.1.7 REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
if(NOT USE_SYSTEM_GZLIBS)
    include_directories(${GZLIBS_INSTALL_PREFIX}/include)
    include_directories(${GZLIBS_INSTALL_PREFIX}/include/gz/cmake3)
    include_directories(${GZLIBS_INSTALL_PREFIX}/include/gz/math7)
    include_directories(${GZLIBS_INSTALL_PREFIX}/include/gz/utils2)
    include_directories(${GZLIBS_INSTALL_PREFIX}/include/gz/sdformat13)
endif()

coppeliasim_generate_stubs(${CMAKE_CURRENT_BINARY_DIR}/generated XML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/callbacks.xml LUA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/simSDF.lua)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(SOURCES
    plugin.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/3Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/3X3Matrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/4Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/4X4Matrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/7Vector.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/mXnMatrix.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simMath/mathFuncs.cpp
)
coppeliasim_add_plugin(simSDF SOURCES ${SOURCES})
target_compile_definitions(simSDF PRIVATE SIM_MATH_DOUBLE)
target_link_libraries(simSDF Boost::boost Boost::filesystem)
if(USE_SYSTEM_GZLIBS)
    target_link_libraries(simSDF gz-math7::gz-math7)
    target_link_libraries(simSDF sdformat13::sdformat13)
else()
    add_dependencies(simSDF ext-sdformat)
    target_link_directories(simSDF PRIVATE ${GZLIBS_INSTALL_PREFIX}/lib)
    target_link_libraries(simSDF gz-utils2 gz-math7 sdformat13)
    install(CODE "file(REMOVE_RECURSE ${GZLIBS_INSTALL_PREFIX}/lib/cmake ${GZLIBS_INSTALL_PREFIX}/lib/ruby ${GZLIBS_INSTALL_PREFIX}/lib/pkgconfig)")
    install(CODE "file(INSTALL ${GZLIBS_INSTALL_PREFIX}/lib/ DESTINATION ${COPPELIASIM_LIBRARIES_DIR} FILES_MATCHING PATTERN \"*${CMAKE_SHARED_LIBRARY_SUFFIX}*\")")
endif()
coppeliasim_add_addon("simAddOnSDF importer.lua")
