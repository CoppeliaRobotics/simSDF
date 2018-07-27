cmake_minimum_required(VERSION 2.8.11)
project(v_repExtSDF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_MACOSX_RPATH 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

if(${CMAKE_VERSION} VERSION_LESS 3.0)
    qt5_wrap_ui(FORMS SDFDialog.ui)
    add_custom_target(uic_cmake28_compat ALL DEPENDS ${FORMS})
endif()

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CoppeliaRobotics-modules)
find_package(VREP 3.5.0 REQUIRED)

include_directories(${VREP_INCLUDE})
include_directories(${VREP_ROOT}/programming/v_repMath)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
include_directories(${Boost_INCLUDE_DIRS})

vrep_generate_stubs(${CMAKE_CURRENT_BINARY_DIR}/generated XML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/callbacks.xml LUA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/simExtSDF.lua)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/external/v_repPlusPlus/Plugin.cpp
    ${VREP_ROOT}/programming/v_repMath/3Vector.cpp
    ${VREP_ROOT}/programming/v_repMath/3X3Matrix.cpp
    ${VREP_ROOT}/programming/v_repMath/4Vector.cpp
    ${VREP_ROOT}/programming/v_repMath/4X4FullMatrix.cpp
    ${VREP_ROOT}/programming/v_repMath/4X4Matrix.cpp
    ${VREP_ROOT}/programming/v_repMath/6Vector.cpp
    ${VREP_ROOT}/programming/v_repMath/6X6Matrix.cpp
    ${VREP_ROOT}/programming/v_repMath/7Vector.cpp
    ${VREP_ROOT}/programming/v_repMath/MMatrix.cpp
    ${VREP_ROOT}/programming/v_repMath/MyMath.cpp
    ${VREP_ROOT}/programming/v_repMath/Vector.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated/stubs.cpp
    ${VREP_EXPORTED_SOURCES}
)
add_library(v_repExtSDF SHARED ${SOURCES})
target_link_libraries(v_repExtSDF ${Boost_LIBRARIES} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport ${VREP_LIBRARIES})

install(TARGETS v_repExtSDF DESTINATION ${VREP_ROOT})

