project(Knossos)
cmake_minimum_required(VERSION 2.8.11)

if(NOT CMAKE_BUILD_TYPE)#prevent accidental debug builds
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CustomLibraryLocations.cmake)

#the *.cpp file (why ever its named *.cpp) is skipped because it causes problems
file(GLOB openjpeg ./openjpeg/*.c ./openjpeg/*.h)

file(GLOB root *.cpp *.h)
file(GLOB_RECURSE widgets ./widgets/*.cpp ./widgets/*.h)
set(SRC_LIST ${openjpeg} ${root} ${widgets})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(CURL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)

if(WIN32)
    #statically link to curl
    add_definitions(-DCURL_STATICLIB)
    set(CURL_LIBRARIES ${CURL_LIBRARIES} -lWs2_32)
    #statically link to glut
    string(REGEX REPLACE "\\.dll" "" GLUT_LIBRARIES ${GLUT_LIBRARIES})
    add_definitions(-DFREEGLUT_STATIC -DGLUT_STATIC_LIB)
    set(GLUT_LIBRARIES ${GLUT_LIBRARIES} -lWinmm)
endif()

#i refrained from creating an external FindTurboJPEG.cmake
#a TurboJPEG-config.cmake in the install directory would be nice
find_path(TURBOJPEG_INCLUDE_DIR turbojpeg.h REQUIRED)
set(turbojpeg_static "libturbojpeg.a")#prefer static lib
find_library(TURBOJPEG_LIBRARY NAMES ${turbojpeg_static} turbojpeg PATHS "C:/libjpeg-turbo-gcc" "C:/libjpeg-turbo-gcc64")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TURBOJPEG REQUIRED_VARS TURBOJPEG_INCLUDE_DIR TURBOJPEG_LIBRARY)

add_library(TurboJPEG STATIC IMPORTED)
set_property(TARGET TurboJPEG PROPERTY IMPORTED_LOCATION ${TURBOJPEG_LIBRARY})
include_directories(TURBOJPEG_INCLUDE_DIR)
#enable turbojpeg inside knossos
add_definitions(-DKNOSSOS_USE_TURBOJPEG)


find_package(Qt5Core 5.1 REQUIRED)
find_package(Qt5Gui 5.1 REQUIRED)
find_package(Qt5Help 5.1 REQUIRED)
find_package(Qt5OpenGL 5.1 REQUIRED)
find_package(Qt5Widgets 5.1 REQUIRED)
#qt resources
qt5_add_resources(CompiledResourceFiles Resources.qrc)
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
    COMMAND qcollectiongenerator ${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhcp -o ${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc
    DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhcp"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/doc.qhp"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/documentation.html"
)


add_executable(${PROJECT_NAME} ${SRC_LIST} ${CompiledResourceFiles})
add_dependencies(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc)
include_directories(
    ${CURL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME}
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
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")


#set(CMAKE_CXX_COMPILER "clang++")#for convenience: switch to clang if you wish
if(CMAKE_CXX_COMPILER MATCHES "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")#use clangs own std library
    target_link_libraries(${PROJECT_NAME} c++abi)#available abi implementation differs per system
endif()
