cmake_minimum_required(VERSION 3.0.0)
project(simExtSDF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_MACOSX_RPATH 1)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
endif()

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

if(${CMAKE_VERSION} VERSION_LESS 3.0)
    qt5_wrap_ui(FORMS SDFDialog.ui)
    add_custom_target(uic_cmake28_compat ALL DEPENDS ${FORMS})
endif()

if(NOT LIBPLUGIN_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(LIBPLUGIN_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/libPlugin)
    else()
        set(LIBPLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libPlugin)
    endif()
endif()
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${LIBPLUGIN_DIR}/cmake)
find_package(CoppeliaSim 4.0.0.0 REQUIRED)

include_directories(${COPPELIASIM_INCLUDE_DIR})
include_directories(${COPPELIASIM_ROOT_DIR}/programming/simMath)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
include_directories(${Boost_INCLUDE_DIRS})

coppeliasim_generate_stubs(${CMAKE_CURRENT_BINARY_DIR}/generated XML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/callbacks.xml LUA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/simExtSDF.lua)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(SOURCES
    ImportOptions.cpp
    SDFDialog.cpp
    SDFParser.cpp
    UIFunctions.cpp
    UIProxy.cpp
    debug.cpp
    tinyxml2.cpp
    plugin.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/3Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/3X3Matrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/4Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/4X4FullMatrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/4X4Matrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/6Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/6X6Matrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/7Vector.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/MMatrix.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/MyMath.cpp
    ${COPPELIASIM_ROOT_DIR}/programming/simMath/Vector.cpp
    ${COPPELIASIM_EXPORTED_SOURCES}
)
add_library(simExtSDF SHARED ${SOURCES})
target_link_libraries(simExtSDF ${Boost_LIBRARIES} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport ${COPPELIASIM_LIBRARIES})

install(TARGETS simExtSDF DESTINATION ${COPPELIASIM_PLUGINS_DIR})

