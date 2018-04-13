#[[
    This file is a part of KNOSSOS.

    (C) Copyright 2007-2018
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
cmake_minimum_required(VERSION 3.8)
project(knossos)

if(NOT CMAKE_BUILD_TYPE)#prevent accidental debug builds
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()
message(STATUS "${CMAKE_BUILD_TYPE}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules")

option(AUTOGEN "use CMAKE_AUTOMOC and CMAKE_AUTORCC instead of manual qt_wrap_cpp and qt5_add_resources" ON)

# find static qt libs (default msys2 location), MINGW_PREFIX is /mingw??
if(WIN32 AND DEFINED BUILD_SHARED_LIBS AND NOT BUILD_SHARED_LIBS)
    message(STATUS "$ENV{MINGW_PREFIX} static build")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{MINGW_PREFIX}/qt5-static/")
endif()

# find system python dll
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SystemRoot}/System32")
elseif(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SystemRoot}/SysWOW64")
endif()

include(CustomLibraryLocations.cmake)#prefill variables

file(GLOB sources *.cpp resources.qrc version.h.in)
file(GLOB headers *.h)
file(GLOB uis widgets/*.ui)
file(GLOB_RECURSE sources2 ./mesh/*.cpp ./scriptengine/*.cpp ./segmentation/*.cpp ./slicer/*.cpp ./skeleton/*.cpp ./tinyply/*.cpp ./widgets/*.cpp)
file(GLOB_RECURSE headers2 ./mesh/*.h ./scriptengine/*.h ./segmentation/*.h ./slicer/*.h ./skeleton/*.h ./tinyply/*.h ./widgets/*.h)

set(SRC_LIST ${sources} ${sources2} ${headers} ${headers2} ${uis})

set(cheatsheet_dir ${CMAKE_CURRENT_BINARY_DIR}/cheatsheet)
list(APPEND SRC_LIST ${cheatsheet_dir}/cheatsheet.qrc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ${AUTOGEN})
set(CMAKE_AUTORCC ${AUTOGEN})#resource files
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOSTATICPLUGINS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 5.9 REQUIRED COMPONENTS Concurrent Core Gui Help Network Widgets) # setWindowFlag

get_target_property(QtCoreLibraryType Qt5::Core TYPE)
if(BUILD_SHARED_LIBS AND ${QtCoreLibraryType} MATCHES STATIC_LIBRARY)
    message(STATUS "static Qt found, disabling BUILD_SHARED_LIBS")
    set(BUILD_SHARED_LIBS OFF) # changing this later wont affect already cached libraries
endif()
# only introduce option (with default value) after testing if it was set initially
option(BUILD_SHARED_LIBS "disable to find static versions of all libraries" ON)
if(NOT BUILD_SHARED_LIBS AND NOT ${QtCoreLibraryType} MATCHES STATIC_LIBRARY)
    message(FATAL_ERROR "BUILD_SHARED_LIBS has been set to FALSE, but the Qt version found is not static")
endif()
if(NOT BUILD_SHARED_LIBS) # try to force search for static libraries from here on if BUILD_SHARED_LIBS was disabled
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES}) # prefer static (*.a) over import (*.dll.a) libraries
endif()

set(pythonqt Qt5Python27 CACHE STRING "PythonQt flavour to search for")
find_package(Boost 1.50 REQUIRED) # 1.50 added hash_value of std::tuple for Coordinate hash specialization
find_package(OpenGL REQUIRED) # GLU
find_package(${pythonqt} REQUIRED)
find_package(Snappy REQUIRED)
find_package(QuaZip 0.6.2 REQUIRED)

if(NOT AUTOGEN)
    qt_wrap_cpp(${PROJECT_NAME} SRC_LIST ${headers} ${headers2})
    qt5_add_resources(SRC_LIST resources.qrc)
    qt5_add_resources(SRC_LIST ${cheatsheet_dir}/cheatsheet.qrc OPTIONS "--root;/cheatsheet")
endif()
#platform dependent resources
if(WIN32)
    set(SRC_LIST "${SRC_LIST}" "${CMAKE_CURRENT_SOURCE_DIR}/resources/knossos.rc")# add windows icon and manifest
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/resources/knossos.rc" PROPERTIES
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

# rcc --project adds all content of a directory to a new .qrc, so create folder for cheatsheet pages.
add_custom_command(OUTPUT ${cheatsheet_dir}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${cheatsheet_dir}
    COMMENT "Creating directory: cheatsheet"
)
add_custom_target(cheatsheet_dir_target DEPENDS ${cheatsheet_dir}) # output is used as input several times

file(GLOB cheatsheet_pages resources/cheatsheet/*.html.in)
add_custom_target(cheatsheet_html)
foreach(page ${cheatsheet_pages})
    get_filename_component(page_name ${page} NAME_WE) #name without path or extension
    add_custom_command(OUTPUT ${cheatsheet_dir}/${page_name}.html
        COMMAND "${CMAKE_CXX_COMPILER}" -E -x c++ -P ${page} -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${cheatsheet_dir}/${page_name}.html # -E: only preprocess, -x: language, -P: omit linemarkers
        COMMENT "Preprocessing ${page}"
        DEPENDS ${page} cheatsheet_dir_target
    )
    add_custom_target(cheatsheet${page_name} DEPENDS ${cheatsheet_dir}/${page_name}.html) # extra add_custom_command prevents infinite loop when running cmake --build
    add_dependencies(cheatsheet_html cheatsheet${page_name})
endforeach()

add_custom_command(OUTPUT ${cheatsheet_dir}/cheatsheet.qrc
    COMMAND ${CMAKE_COMMAND} -E remove -f cheatsheet.qrc # remove old .qrc, otherwise it will be added to new .qrc
    COMMAND Qt5::rcc --project -o ../cheatsheet.qrc # need to create in different folder otherwise .qrc will contain itself. Yep…
    COMMAND ${CMAKE_COMMAND} -E rename ../cheatsheet.qrc ${cheatsheet_dir}/cheatsheet.qrc
    WORKING_DIRECTORY ${cheatsheet_dir}
    COMMENT "Creating cheatsheet.qrc"
    DEPENDS cheatsheet_html
)

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SRC_LIST} buildinfo.h)
set_source_files_properties(${cheatsheet_dir}/cheatsheet.qrc PROPERTIES
    GENERATED ON
    AUTORCC_OPTIONS "--root;/cheatsheet"
)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS ${cheatsheet_dir}/cheatsheet.qrc)

set_target_properties(${PROJECT_NAME}
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist)
add_dependencies(${PROJECT_NAME} buildinfo)#main target needs buildinfo

option(PythonQt_QtAll "Include the PythonQt QtAll extension which wraps all Qt libraries" ON)
if(PythonQt_QtAll)
    find_package(${pythonqt}_QtAll REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "QtAll")
    target_link_libraries(${PROJECT_NAME} ${pythonqt}_QtAll)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT DEPLOY)
    message(STATUS "using gold linker")
    set(LINUXLINKER -fuse-ld=gold)
endif()

target_link_libraries(${PROJECT_NAME}
    Boost::boost
    OpenGL::GLU
    Qt5::Concurrent
    Qt5::Core
    Qt5::Gui
    Qt5::Help
    Qt5::Network
    Qt5::Widgets
    ${pythonqt}
    QuaZip::QuaZip
    Snappy::Snappy
    ${LINUXLINKER}
    $<$<PLATFORM_ID:Windows>:-Wl,--dynamicbase># use ASLR, required by the »Windows security features test« for »Windows Desktop App Certification«
)
# remove the DSServicePlugin as it will depend on multimedia libraries (i.e. evc.dll) only available in non-N editions of Windows
get_target_property(qtmultimedia_static_plugins Qt5::Multimedia STATIC_PLUGINS)
if(qtmultimedia_static_plugins)
    string(REGEX REPLACE [;]*DSServicePlugin[;]* ";" qtmultimedia_static_plugins_new "${qtmultimedia_static_plugins}")
    set_target_properties(Qt5::Multimedia PROPERTIES STATIC_PLUGINS "${qtmultimedia_static_plugins_new}")
endif()

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
endif()

include(cotire OPTIONAL)#https://github.com/sakra/cotire/releases/latest
if(COMMAND cotire)
    set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)# recurring https://github.com/Alexpux/MINGW-packages/issues/923
    cotire(${PROJECT_NAME})
endif()
