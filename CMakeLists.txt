#[[
    This file is a part of KNOSSOS.

    (C) Copyright 2007-2016
    Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.

    KNOSSOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 of
    the License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


    For further information, visit https://knossostool.org
    or contact knossos-team@mpimf-heidelberg.mpg.de
]]

project(knossos)
cmake_minimum_required(VERSION 3.1)

if(NOT CMAKE_BUILD_TYPE)#prevent accidental debug builds
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()
message(STATUS "${CMAKE_BUILD_TYPE}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules")

option(BUILD_SHARED_LIBS "disable to find static versions of all libraries" ON)
if(NOT BUILD_SHARED_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})#prefer static (*.a) over import (*.dll.a) libraries
endif()
option(AUTOGEN "use CMAKE_AUTOMOC and CMAKE_AUTORCC instead of manual qt_wrap_cpp and qt5_add_resources (less cpu usage, worse parallelizability, worse turnaround times)" OFF)

include(CustomLibraryLocations.cmake)#prefill variables
include(cotire OPTIONAL)#https://github.com/sakra/cotire/releases/latest

file(GLOB sources *.cpp resources.qrc version.h.in)
file(GLOB headers *.h)
file(GLOB uis widgets/*.ui)
file(GLOB_RECURSE sources2 ./mesh/*.cpp ./scriptengine/*.cpp ./segmentation/*.cpp ./slicer/*.cpp ./skeleton/*.cpp ./tinyply/*.cpp ./widgets/*.cpp)
file(GLOB_RECURSE headers2 ./mesh/*.h ./scriptengine/*.h ./segmentation/*.h ./slicer/*.h ./skeleton/*.h ./tinyply/*.h ./widgets/*.h)

set(SRC_LIST ${sources} ${sources2} ${headers} ${headers2} ${uis})

set(CMAKE_CXX_STANDARD 14)
if(NOT COMMAND cotire)#https://github.com/sakra/cotire/issues/99
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()
set(CMAKE_AUTOMOC ${AUTOGEN})
set(CMAKE_AUTORCC ${AUTOGEN})#resources.qrc
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOSTATICPLUGINS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(pythonqt Qt5Python27 CACHE STRING "PythonQt flavour to search for")
find_package(Boost 1.50 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(${pythonqt} REQUIRED)
find_package(Snappy REQUIRED)
find_package(Qt5 5.1 REQUIRED COMPONENTS Concurrent Core Gui Help Network Widgets)
find_package(QuaZip 0.6.2 REQUIRED)

if(NOT AUTOGEN)
    qt_wrap_cpp(${PROJECT_NAME} SRC_LIST ${headers} ${headers2})
    qt5_add_resources(SRC_LIST resources.qrc)
endif()

get_target_property(QtCoreLibraryType Qt5::Core TYPE)
if(NOT BUILD_SHARED_LIBS AND NOT ${QtCoreLibraryType} MATCHES STATIC_LIBRARY)
    message(FATAL_ERROR "BUILD_SHARED_LIBS has been set to FALSE, but the Qt version found is not static")
endif()

#platform dependent resources
if(WIN32)
    set(SRC_LIST "${SRC_LIST}" "${CMAKE_CURRENT_SOURCE_DIR}/knossos.rc")# add windows icon and manifest
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/knossos.rc" PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/knossos.exe.manifest;${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/logo.ico")
elseif(APPLE)
    set_source_files_properties("resources/icons/knossos.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(SRC_LIST "${SRC_LIST}" "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/knossos.icns")
endif()

add_custom_target(buildinfo DEPENDS buildinfocreator)#forces the pseudo target to be generated
#is always triggered by the pseudo target which is never actually created, creates buildinfo.h instead
add_custom_command(
    OUTPUT
        buildinfocreator
        ${CMAKE_CURRENT_BINARY_DIR}/buildinfo.h
    COMMAND ${CMAKE_COMMAND}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/buildinfo.h.in
        -D DST=${CMAKE_CURRENT_BINARY_DIR}/buildinfo.h
        -D GIT=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/buildinfo.cmake
)


add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SRC_LIST} buildinfo.h)
set_target_properties(${PROJECT_NAME}
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist)
add_dependencies(${PROJECT_NAME} buildinfo)#main target needs buildinfo

option(PythonQt_QtAll "Include the PythonQt QtAll extension which wraps all Qt libraries" ON)
if(PythonQt_QtAll)
    find_package(${pythonqt}_QtAll REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "QtAll")
    target_link_libraries(${PROJECT_NAME} ${pythonqt}_QtAll)
endif()
#on static build, need to make sure the root include of a dynamic Qt is not added i.e. /mingw64/include
#or some headers files of the static Qt are resolved through the wrong Qt installation (i.e. QtCore/qglobal.h which defines QT_STATIC)
target_include_directories(${PROJECT_NAME} PRIVATE
    #${Boost_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${OPENGL_LIBRARIES}
    #configs add includes and other settings automatically
    Qt5::Concurrent
    Qt5::Core
    Qt5::Gui
    Qt5::Help
    Qt5::Network
    Qt5::Widgets
    ${pythonqt}
    QuaZip::QuaZip
    Snappy::Snappy
    $<$<PLATFORM_ID:Linux>:-fuse-ld=gold>
    $<$<PLATFORM_ID:Windows>:-Wl,--dynamicbase># use ASLR, required by the »Windows security features test« for »Windows Desktop App Certification«
)
find_program(LSBRELEASE lsb_release)
if(NOT ${LSBRELEASE} STREQUAL "LSBRELASE-NOTFOUND")
    execute_process(COMMAND ${LSBRELEASE} -is
        OUTPUT_VARIABLE LSB_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if("${LSB_OUTPUT}" STREQUAL "Ubuntu")
        target_link_libraries(${PROJECT_NAME} xcb Xxf86vm dl)
    endif()
endif()
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
else(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endif(APPLE)

#uncomment if you are to lazy to do »ninja install« to get the RPATH
set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

target_compile_options(${PROJECT_NAME} PRIVATE "-pedantic-errors")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE "-fcolor-diagnostics")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE "-fdiagnostics-color=always")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER_CASE)
if(CMAKE_BUILD_TYPE_UPPER_CASE MATCHES "DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG_PEDANTIC")
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_BUILD_TYPE_UPPER_CASE MATCHES "RELEASE")#no openmp for clang
    find_package(OpenMP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_PARALLEL")
    target_compile_options(${PROJECT_NAME} PRIVATE "-floop-parallelize-all" "-ftree-parallelize-loops=4")
endif()

if(COMMAND cotire)
    cotire(${PROJECT_NAME})
endif()
