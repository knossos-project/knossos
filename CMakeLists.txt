project(knossos)
cmake_minimum_required(VERSION 3.0)

if(NOT CMAKE_BUILD_TYPE)#prevent accidental debug builds
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

include(CustomLibraryLocations.cmake)#prefill variables

#the *.cpp file of openjpeg (why ever its named *.cpp) is skipped because it causes problems
file(GLOB openjpeg ./openjpeg/*.c ./openjpeg/*.h)

file(GLOB root *.cpp *.h *.qrc)
file(GLOB_RECURSE widgets ./widgets/*.cpp ./widgets/*.h)
set(SRC_LIST ${openjpeg} ${root} ${widgets})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)#Resources.qrc
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(KNOSSOS_STATIC_LIBRARIES OFF CACHE BOOL "try to find static versions of all libraries")
if(KNOSSOS_STATIC_LIBRARIES)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})#prefer static (*.a) over import (*.dll.a) libraries
endif()

find_package(Boost REQUIRED)
find_package(CURL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)

if(WIN32)
    set(CURL_LIBRARIES ${CURL_LIBRARIES} -lWs2_32)#this is apparently always neccessary
    if(KNOSSOS_STATIC_LIBRARIES)
        #these definitions remove the dllimport directives from the includes
        add_definitions(-DCURL_STATICLIB)
        add_definitions(-DFREEGLUT_STATIC -DGLUT_STATIC_LIB)
        #satisfy additional dependecies
        set(GLUT_LIBRARIES ${GLUT_LIBRARIES} -lWinmm)
        add_definitions(-DQUAZIP_STATIC)
    endif()
endif()

#i refrained from creating an external FindTurboJPEG.cmake
#but a TurboJPEG-config.cmake in the install directory would be nice
find_path(TURBOJPEG_INCLUDE_DIR turbojpeg.h REQUIRED)
find_library(TURBOJPEG_LIBRARY turbojpeg)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TURBOJPEG DEFAULT_MSG TURBOJPEG_INCLUDE_DIR TURBOJPEG_LIBRARY)

add_library(TurboJPEG STATIC IMPORTED)
set_property(TARGET TurboJPEG PROPERTY IMPORTED_LOCATION ${TURBOJPEG_LIBRARY})
include_directories(${TURBOJPEG_INCLUDE_DIR})
#enable turbojpeg inside knossos
add_definitions(-DKNOSSOS_USE_TURBOJPEG)


find_package(Qt5 5.1 REQUIRED COMPONENTS Core Gui Help OpenGL Widgets)
find_package(QuaZip REQUIRED)
include_directories(${QUAZIP_INCLUDE_DIRS})
#platform dependent resources
if(WIN32)
    set(SRC_LIST "${SRC_LIST}" "${CMAKE_CURRENT_SOURCE_DIR}/knossos.rc")#add windows icon
elseif(APPLE)
    set_source_files_properties("knossos.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()
#generate docs
add_custom_command(
    OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/doc.qch"
    PRE_BUILD
    COMMAND qcollectiongenerator "${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhcp" -o "${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc"
    DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhcp"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/doc.qhp"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/documentation.html"
)


add_executable(${PROJECT_NAME} ${SRC_LIST})
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc" "${CMAKE_CURRENT_SOURCE_DIR}/doc/doc.qch")
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    #configs add includes and other settings automatically
    TurboJPEG
    Qt5::Core
    Qt5::Gui
    Qt5::Help
    Qt5::OpenGL
    Qt5::Widgets
    ${QUAZIP_LIBRARIES}
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

#set(CMAKE_C_COMPILER "clang")
#set(CMAKE_CXX_COMPILER "clang++")
if(CMAKE_CXX_COMPILER MATCHES "clang")#sadly there’s no recent libc++ for travis
    if(DEFINED ENV{TRAVIS})#there’s no libc++ for 12.04
        #(stupid) clang #defines (wrongly) disable boost features
        add_definitions(-D__GNUC_MINOR__=8)#evil workaround
        #see https://svn.boost.org/trac/boost/ticket/7473
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")#use clangs own std library
        target_link_libraries(${PROJECT_NAME} c++abi)#available abi implementation differs per system
    endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
    add_definitions(-D_GLIBCXX_DEBUG)
    add_definitions(-D_GLIBCXX_DEBUG_PEDANTIC)
elseif(NOT CMAKE_CXX_COMPILER MATCHES "clang" AND CMAKE_BUILD_TYPE MATCHES "RELEASE")#no openmp for clang
    add_definitions(-D_GLIBCXX_PARALLEL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -floop-parallelize-all -ftree-parallelize-loops=4")
endif()
