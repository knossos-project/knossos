project(knossos)
cmake_minimum_required(VERSION 3.0)

if(NOT CMAKE_BUILD_TYPE)#prevent accidental debug builds
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

option(BUILD_SHARED_LIBS "disable to find static versions of all libraries" ON)
if(NOT BUILD_SHARED_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})#prefer static (*.a) over import (*.dll.a) libraries
endif()

include(CustomLibraryLocations.cmake)#prefill variables

#the *.cpp file of openjpeg (why ever its named *.cpp) is skipped because it causes problems
file(GLOB openjpeg ./openjpeg/*.c ./openjpeg/*.h)

file(GLOB root *.cpp *.h *.qrc *.h.in)
file(GLOB_RECURSE scriptengine ./scriptengine/*.cpp ./scriptengine/*.h)
file(GLOB_RECURSE skeleton ./skeleton/*.cpp ./skeleton/*.h)
file(GLOB_RECURSE segmentation ./segmentation/*.cpp ./segmentation/*.h)
file(GLOB_RECURSE widgets ./widgets/*.cpp ./widgets/*.h)
file(GLOB_RECURSE docimages ./resources/doc/images/*)
set(SRC_LIST ${openjpeg} ${root} ${segmentation} ${scriptengine} ${skeleton} ${widgets})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)#resources.qrc
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost 1.50 REQUIRED)
find_package(CURL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Qt5Python27 REQUIRED)
find_package(Snappy REQUIRED)
find_package(TurboJPEG REQUIRED)
find_package(Qt5 5.1 REQUIRED COMPONENTS Core Gui Help Network OpenGL Sql Widgets Xml)
find_package(QuaZip 0.6.2 REQUIRED)

#platform dependent resources
if(WIN32)
    set(SRC_LIST "${SRC_LIST}" "${CMAKE_CURRENT_SOURCE_DIR}/knossos.rc")#add windows icon
elseif(APPLE)
    set_source_files_properties("resources/icons/knossos.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

#generate docs
find_program(QCOLLECTIONGENERATOR qcollectiongenerator)
add_custom_command(
    OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/collection.qhc"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/doc.qch"
    PRE_BUILD
    COMMAND ${QCOLLECTIONGENERATOR} "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/collection.qhcp" -o "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/collection.qhc"
    DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/collection.qhcp"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/doc.qhp"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/documentation.html"
    ${docimages}
)

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


add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    ${SRC_LIST} buildinfo.h ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/knossos.icns)
set_target_properties(${PROJECT_NAME}
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist)
add_dependencies(${PROJECT_NAME} buildinfo)#main target needs buildinfo
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/collection.qhc" "${CMAKE_CURRENT_SOURCE_DIR}/resources/doc/doc.qch")
#custom adjustments
if(NOT BUILD_SHARED_LIBS)
	set(CMAKE_AUTOSTATICPLUGINS ON)#https://github.com/Alexpux/MINGW-packages/issues/577
    #these definitions remove the dllimport directives from the includes
    target_compile_definitions(${PROJECT_NAME} PRIVATE "CURL_STATICLIB")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "FREEGLUT_STATIC" "GLUT_STATIC_LIB")
    find_program(CURL_CONFIG curl-config)
    execute_process(COMMAND ${CURL_CONFIG} --static-libs
        OUTPUT_VARIABLE CURL_CONFIG_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT "${CURL_CONFIG_OUTPUT}" STREQUAL "")
        if(NOT APPLE)
            string(REPLACE " " ";" CURL_LIBRARIES "${CURL_CONFIG_OUTPUT}")#convert to list
            set(CURL_CONFIG_OUTPUT "")
        else()
            set(CURL_LIBRARIES "")
        endif()
    endif()
    if(WIN32)
        set(CURL_LIBRARIES ${CURL_LIBRARIES} -lidn -lssl -lcrypto -lintl -liconv -lWs2_32)
        set(GLUT_LIBRARIES freeglut_static -lWinmm)
    endif()
endif()
if(WIN32)
    set(CURL_LIBRARIES ${CURL_LIBRARIES} -lWs2_32)
endif()
option(PythonQt_QtAll "Include the PythonQt QtAll extension which wraps all Qt libraries" ON)
if(PythonQt_QtAll)
    find_package(Qt5Python27_QtAll REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "QtAll")
    target_link_libraries(${PROJECT_NAME} Qt5Python27_QtAll)
endif()
#on static build, need to make sure the root include of a dynamic Qt is not added i.e. /mingw64/include
#or some headers files of the static Qt are resolved through the wrong Qt installation (i.e. QtCore/qglobal.h which defines QT_STATIC)
target_include_directories(${PROJECT_NAME} PRIVATE
    #${Boost_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    #${GLUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    #configs add includes and other settings automatically
    TurboJPEG::TurboJPEG#turbojpeg is partly included in QtGui, multiple definition if you try to link it afterwards
    Qt5::Core
    Qt5::Gui
    Qt5::Help
    Qt5::OpenGL
    Qt5::Sql
    Qt5::Widgets
    Qt5::Network
    Qt5Python27
    QuaZip::QuaZip
    Snappy::Snappy
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
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${CURL_CONFIG_OUTPUT}")
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
#uncomment if you are to lazy to do »ninja install« to get the RPATH
set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#target_compile_options(${PROJECT_NAME} PRIVATE "-fcolor-diagnostics")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra" "-Wpedantic")

#set(CMAKE_C_COMPILER "clang")
#set(CMAKE_CXX_COMPILER "clang++")

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_DEBUG_PEDANTIC")
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_BUILD_TYPE MATCHES "RELEASE")#no openmp for clang
    find_package(OpenMP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_GLIBCXX_PARALLEL")
    target_compile_options(${PROJECT_NAME} PRIVATE "-floop-parallelize-all" "-ftree-parallelize-loops=4")
endif()
