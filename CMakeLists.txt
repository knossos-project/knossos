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


    For further information, visit https://knossos.app
    or contact knossosteam@gmail.com
]]
cmake_minimum_required(VERSION 3.24)
project(knossos CXX)

if(NOT CMAKE_BUILD_TYPE)#prevent accidental debug builds
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()
message(STATUS "${CMAKE_BUILD_TYPE}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules")

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

file(GLOB sources *.cpp resources.qrc)
file(GLOB headers *.h)
file(GLOB uis widgets/*.ui)
file(GLOB_RECURSE sources2 ./annotation/*.cpp ./mesh/*.cpp ./scriptengine/*.cpp ./segmentation/*.cpp ./slicer/*.cpp ./skeleton/*.cpp ./tinyply/*.cpp ./widgets/*.cpp)
file(GLOB_RECURSE headers2 ./annotation/*.h ./mesh/*.h ./scriptengine/*.h ./segmentation/*.h ./slicer/*.h ./skeleton/*.h ./tinyply/*.h ./widgets/*.h)

set(SRC_LIST ${sources} ${sources2} ${headers} ${headers2} ${uis})

set(cheatsheet_dir ${CMAKE_CURRENT_BINARY_DIR}/cheatsheet)
list(APPEND SRC_LIST ${cheatsheet_dir}/cheatsheet.qrc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)#resource files
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_property(SOURCE ${sources};${sources2} PROPERTY SKIP_AUTOMOC ON)

if(NOT BUILD_SHARED_LIBS) # try to force search for static libraries if BUILD_SHARED_LIBS was disabled
    list(PREPEND CMAKE_FIND_LIBRARY_SUFFIXES .a .lib) # prefer static (*.a) over import (*.dll.a) libraries
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Concurrent Core Gui Network Widgets CONFIG) # QJsonValue::operator[]

get_target_property(QtCoreLinkLibraries Qt::Core INTERFACE_LINK_LIBRARIES)
get_target_property(QtCoreLibraryType ${QtCoreLinkLibraries} TYPE)
if(BUILD_SHARED_LIBS AND ${QtCoreLibraryType} MATCHES STATIC_LIBRARY)
    message(STATUS "static Qt found, disabling BUILD_SHARED_LIBS")
    set(BUILD_SHARED_LIBS OFF) # changing this later wont affect already cached libraries
endif()
# only introduce option (with default value) after testing if it was set initially
option(BUILD_SHARED_LIBS "disable to find static versions of all libraries" ON)
if(NOT BUILD_SHARED_LIBS AND NOT ${QtCoreLibraryType} MATCHES STATIC_LIBRARY)
    message(FATAL_ERROR "BUILD_SHARED_LIBS has been set to FALSE, but the Qt version found is not static")
endif()

find_package(Python COMPONENTS Development.Embed)
set(pythonqt Qt5Python3${Python_VERSION_MINOR} CACHE STRING "PythonQt flavour to search for")
find_package(Boost 1.50 REQUIRED) # 1.50 added hash_value of std::tuple for Coordinate hash specialization
find_package(OpenGL REQUIRED COMPONENTS OpenGL) # GLU
find_package(${pythonqt} REQUIRED CONFIG)
find_package(Snappy REQUIRED MODULE)
find_package(QuaZip 0.6.2 REQUIRED MODULE)

include(FetchContent)
FetchContent_Declare(toml11 GIT_REPOSITORY https://github.com/ToruNiina/toml11.git)
FetchContent_Populate(toml11)

add_library(toml11::toml11 INTERFACE IMPORTED)
target_include_directories(toml11::toml11 INTERFACE "${toml11_SOURCE_DIR}")

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
#is always triggered by the pseudo target which is never actually created, creates buildinfo.cpp instead
add_custom_command(
    OUTPUT
        buildinfocreator
        ${CMAKE_CURRENT_BINARY_DIR}/buildinfo.cpp
    COMMAND ${CMAKE_COMMAND}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/buildinfo.cpp.in
        -D DST=${CMAKE_CURRENT_BINARY_DIR}/buildinfo.cpp
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
foreach(page ${cheatsheet_pages})
    get_filename_component(page_name ${page} NAME_WE) #name without path or extension
    add_custom_command(OUTPUT ${cheatsheet_dir}/${page_name}.html
        COMMAND ${CMAKE_COMMAND} -E copy ${page} ${cheatsheet_dir}/${page_name}.html
        COMMENT "Preprocessing ${page}"
        DEPENDS ${page} cheatsheet_dir_target
    )
    list(APPEND genpages ${cheatsheet_dir}/${page_name}.html)
endforeach()

execute_process(COMMAND ${CMAKE_COMMAND} -E remove -f ${cheatsheet_dir}/cheatsheet.qrc) # missing pages (e.g. on branch switch) break build
add_custom_command(OUTPUT ${cheatsheet_dir}/cheatsheet.qrc
    COMMAND ${CMAKE_COMMAND} -E remove -f cheatsheet.qrc # remove old .qrc, otherwise it will be added to new .qrc
    COMMAND Qt::rcc --project -o ../cheatsheet.qrc # need to create in different folder otherwise .qrc will contain itself. Yep…
    COMMAND ${CMAKE_COMMAND} -E rename ../cheatsheet.qrc ${cheatsheet_dir}/cheatsheet.qrc
    WORKING_DIRECTORY ${cheatsheet_dir}
    COMMENT "Creating cheatsheet.qrc"
    DEPENDS ${genpages}
)

if(NOT MINGW) # msys2-lld __delayLoadHelper2 broken
find_program(LLD lld)
if(LLD)
    message(STATUS "using LLVM linker")
    # CMAKE_COLOR_DIAGNOSTICS doesn’t apply coloring to the compiler driver during linking
    add_link_options(-fuse-ld=lld LINKER:--color-diagnostics -fdiagnostics-color=always)
else()
    message(STATUS "using gold linker")
    add_link_options($<$<PLATFORM_ID:Linux>:-fuse-ld=gold>) # only ELF
endif()
endif()

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SRC_LIST} buildinfo.cpp)
set_source_files_properties(${cheatsheet_dir}/cheatsheet.qrc PROPERTIES
    AUTORCC_OPTIONS "--root;/cheatsheet"
)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS ${cheatsheet_dir}/cheatsheet.qrc)

set_target_properties(${PROJECT_NAME}
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist)
add_dependencies(${PROJECT_NAME} buildinfo)#main target needs buildinfo

option(PythonQt_QtAll "Include the PythonQt QtAll extension which wraps all Qt libraries" ON)
if(PythonQt_QtAll)
    find_package(${pythonqt}_QtAll REQUIRED CONFIG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "QtAll")
    target_link_libraries(${PROJECT_NAME} ${pythonqt}_QtAll)
    # remove the DSServicePlugin as it will depend on multimedia libraries (i.e. evc.dll) only available in non-N editions of Windows
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_NO_PLUGINS Qt::DSServicePlugin)
endif()

target_link_libraries(${PROJECT_NAME}
    Boost::headers
    OpenGL::GLU
    Qt::Concurrent
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
    ${pythonqt}
    QuaZip::QuaZip
    Snappy::Snappy
    toml11::toml11
    $<$<AND:$<PLATFORM_ID:Windows>,$<NOT:$<CONFIG:Debug>>>:-Wl,--dynamicbase># use ASLR, required by the »Windows security features test« for »Windows Desktop App Certification«
)

find_program(LSBRELEASE lsb_release)
if(LSBRELEASE)
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

#target_compile_options(${PROJECT_NAME} PRIVATE "-ftime-report")
target_compile_options(${PROJECT_NAME} PRIVATE "-pedantic-errors")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra")
target_compile_options(${PROJECT_NAME} PRIVATE "-fdiagnostics-show-template-tree")

#list(APPEND CMAKE_CXX_FLAGS "-fsanitize=address")

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER_CASE)
if(CMAKE_BUILD_TYPE_UPPER_CASE MATCHES "DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG_PEDANTIC")
endif()

include(cotire OPTIONAL)#https://github.com/sakra/cotire/releases/latest
if(COMMAND cotire)
    cotire(${PROJECT_NAME})
endif()
