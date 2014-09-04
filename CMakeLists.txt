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
file(GLOB_RECURSE scriptengine ./scriptengine/*.cpp ./scriptengine/*.h)
file(GLOB_RECURSE widgets ./widgets/*.cpp ./widgets/*.h)
set(SRC_LIST ${openjpeg} ${root} ${scriptengine} ${widgets})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)#Resources.qrc
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(BUILD_SHARED_LIBS "try to find static versions of all libraries")
if(NOT BUILD_SHARED_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})#prefer static (*.a) over import (*.dll.a) libraries
endif()

find_package(Boost 1.50 REQUIRED)
find_package(CURL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PythonLibs 2  REQUIRED)#we sadly need to find the python includes ourselves
find_package(PythonQt5 REQUIRED)
find_package(Snappy REQUIRED)
find_package(TurboJPEG REQUIRED)
find_package(Qt5 5.1 REQUIRED COMPONENTS Core Gui Help OpenGL Widgets)
find_package(QuaZip 0.6.2 REQUIRED)

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
#custom adjustments
if(NOT BUILD_SHARED_LIBS)
    #these definitions remove the dllimport directives from the includes
    target_compile_definitions(${PROJECT_NAME} PRIVATE "CURL_STATICLIB")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "FREEGLUT_STATIC" "GLUT_STATIC_LIB")
    if(UNIX AND NOT "${CURL_LIBRARIES}" MATCHES ".so")
        execute_process(COMMAND curl-config --static-libs
            OUTPUT_VARIABLE CURL_LIBRARIES
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        separate_arguments(CURL_LIBRARIES)
    endif()
    if(WIN32)
        set(CURL_LIBRARIES ${CURL_LIBRARIES} -lidn -lssl -lcrypto -lintl -liconv)
        #satisfy additional dependecies
        set(GLUT_LIBRARIES ${GLUT_LIBRARIES} -lWinmm)
        set(CURL_LIBRARIES ${CURL_LIBRARIES} -lWs2_32)#this is apparently always neccessary
    endif()
endif()
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${PythonQt_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    #configs add includes and other settings automatically
    PythonQt5
    TurboJPEG::TurboJPEG
    Qt5::Core
    Qt5::Gui
    Qt5::Help
    Qt5::OpenGL
    Qt5::Widgets
    QuaZip::QuaZip
    Snappy::Snappy
    -lxcb -lXxf86vm -ldl
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra" "-Wpedantic")

#set(CMAKE_C_COMPILER "clang")
#set(CMAKE_CXX_COMPILER "clang++")
if(CMAKE_CXX_COMPILER MATCHES "clang")#sadly there’s no recent libc++ for travis
    if(DEFINED ENV{TRAVIS})#there’s no libc++ for 12.04
        #(stupid) clang #defines (wrongly) disable boost features
        target_compile_definitions(${PROJECT_NAME} PRIVATE "__GNUC_MINOR__=8")#evil workaround
        #see https://svn.boost.org/trac/boost/ticket/7473
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE "-stdlib=libc++")#use clangs own std library
        target_link_libraries(${PROJECT_NAME} "c++abi")#available abi implementation differs per system
    endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG_PEDANTIC")
elseif(NOT CMAKE_CXX_COMPILER MATCHES "clang" AND CMAKE_BUILD_TYPE MATCHES "RELEASE")#no openmp for clang
    find_package(OpenMP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_PARALLEL")
    target_compile_options(${PROJECT_NAME} PRIVATE "-floop-parallelize-all" "-ftree-parallelize-loops=4")
endif()
