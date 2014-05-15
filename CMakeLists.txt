project(knossos)

cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

add_definitions(-DREVISION="\\"git-svn\\"")

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)

include_directories(
    ${GLUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIR}
)

find_package(Qt5Core REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Help REQUIRED)
find_package(Qt5Widgets REQUIRED)

SET(KNOSSOS_SOURCES
    widgets/mainwindow.cpp
    eventmodel.cpp
    loader.cpp
    viewer.cpp
    remote.cpp
    skeletonizer.cpp
    renderer.cpp
    knossos.cpp
    coordinate.cpp
    Hashtable.cpp
    scripting.cpp
    qsort.cpp
    ftp.cpp
    task.cpp
    sha256.cpp
    treeLUT_fallback.cpp
    functions.cpp
#
    widgets/viewport.cpp
    widgets/splashscreenwidget.cpp
    widgets/datasetpropertywidget.cpp
    widgets/documentationwidget.cpp
    widgets/annotationwidget.cpp
    widgets/console.cpp
    widgets/tracingtimewidget.cpp
    widgets/commentswidget.cpp
    widgets/commentshortcuts/commentshortcutstab.cpp
    widgets/commentshortcuts/commentshighlightingtab.cpp
    widgets/zoomandmultireswidget.cpp
    widgets/datasavingwidget.cpp
    widgets/navigationwidget.cpp
    widgets/viewportsettingswidget.cpp
    widgets/widgetcontainer.cpp
    widgets/commentshortcuts/commentsnodecommentstab.cpp
#
    widgets/tools/treetable.cpp
    widgets/tools/toolstreeviewtab.cpp
    widgets/tools/nodetable.cpp
    widgets/tools/toolscommandstab.cpp
    widgets/tools/toolstreeviewtab.cpp
#
    widgets/viewportsettings/vpsliceplaneviewportwidget.cpp
    widgets/viewportsettings/vpskeletonviewportwidget.cpp
    widgets/viewportsettings/vpgeneraltabwidget.cpp
#
    widgets/task/taskloginwidget.cpp
    widgets/task/taskmanagementwidget.cpp
    widgets/task/taskmanagementmaintab.cpp
    widgets/task/taskmanagementdetailstab.cpp
#
    decorators/skeletonizerdecorator.cpp
    decorators/mainwindowdecorator.cpp
)

qt5_add_resources(KNOSSOS_RESOURCES_SOURCES
    Resources.qrc
)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/doc.qhc
    PRE_BUILD
    COMMAND qcollectiongenerator
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhcp
        -o ${CMAKE_CURRENT_SOURCE_DIR}/doc/collection.qhc
)

qt5_generate_moc(knossos-global.h ${CMAKE_CURRENT_BINARY_DIR}/moc_knossos-global.cpp)

add_executable(knossos
    ${KNOSSOS_SOURCES}
    ${KNOSSOS_RESOURCES_SOURCES}
    ${KNOSSOS_UIS}
    ${CMAKE_CURRENT_BINARY_DIR}/moc_knossos-global.cpp
)
get_property(location TARGET knossos PROPERTY LOCATION)
add_custom_command(TARGET knossos
    POST_BUILD        
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  
        ${location}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(knossos
    ${OPENGL_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GLUT_LIBRARIES}
    openjpeg_knossos
)

qt5_use_modules(knossos OpenGL Network Xml Test Help Widgets)

# currently no UI files...
#
# set(KNOSSOS_UIS mainwindow.ui)
# qt5_wrap_ui(KNOSSOS_UIS_HEADERS
#     ${KNOSSOS_UIS}
# )

add_subdirectory(openjpeg)
