#-------------------------------------------------
#
# Project created by QtCreator 2012-09-05T18:04:31
#
#-------------------------------------------------

QT       += core gui opengl network

TARGET = knossos
TEMPLATE = app

SOURCES +=\
    mainwindow.cpp \
    eventmodel.cpp \
    client.cpp \
    loader.cpp \
    viewer.cpp \
    remote.cpp \
    skeletonizer.cpp \
    renderer.cpp \
    knossos.cpp \
    coordinate.cpp \
    Hashtable.cpp \
    sleeper.cpp \
    viewport.cpp \
    treeLUT_fallback.cpp \
    widgets/console.cpp \
    widgets/tracingtimewidget.cpp \
    widgets/commentswidget.cpp \
    widgets/zoomandmultireswidget.cpp \
    widgets/datasavingwidget.cpp \
    widgets/navigationwidget.cpp \
    widgets/viewportsettingswidget.cpp \
    widgets/toolswidget.cpp \
    widgets/tools/toolsquicktabwidget.cpp \
    widgets/tools/toolstreestabwidget.cpp \
    widgets/tools/toolsnodestabwidget.cpp \
    widgets/viewportsettings/vpsliceplaneviewportwidget.cpp \
    widgets/viewportsettings/vpskeletonviewportwidget.cpp \
    widgets/viewportsettings/vpgeneraltabwidget.cpp \
    widgets/synchronizationwidget.cpp \
    widgets/splashscreenwidget.cpp \
    functions.cpp \
    texturizer.cpp \
    slicer.cpp

HEADERS  += mainwindow.h \
    knossos-global.h \
    eventmodel.h \
    client.h \
    loader.h \
    viewer.h \
    remote.h \
    skeletonizer.h \
    renderer.h \
    knossos.h\
    sleeper.h \
    viewport.h \
    widgets/console.h \
    widgets/tracingtimewidget.h \
    widgets/commentswidget.h \
    widgets/zoomandmultireswidget.h \
    widgets/datasavingwidget.h \
    widgets/navigationwidget.h \
    widgets/viewportsettingswidget.h \
    widgets/toolswidget.h \
    widgets/tools/toolsquicktabwidget.h \
    widgets/tools/toolstreestabwidget.h \
    widgets/tools/toolsnodestabwidget.h \
    widgets/viewportsettings/vpsliceplaneviewportwidget.h \
    widgets/viewportsettings/vpskeletonviewportwidget.h \
    widgets/viewportsettings/vpgeneraltabwidget.h \
    widgets/synchronizationwidget.h \
    widgets/splashscreenwidget.h \
    functions.h \
    texturizer.h \
    slicer.h


FORMS    += mainwindow.ui

OTHER_FILES += \
    knossos.layout \
    iconv.dll \
    libfreetype-6.dll \
    libxml2.dll \
    pthreadVC2.dll \
    zlib1.dll \
    icon \
    LICENSE \
    Makefile \
    splash \
    knossos.depend \
    knossos.dev \
    default.lut \
    gmon.out \
    knossos.res \
    knossos_private.res \
    knossos.rc \
    knossos_private.rc \
    logo.ico \
    ChangeLog.txt \
    defaultSettings.xml \
    customCursor.xpm \
    config.y \

LIBS += -lxml2

INCLUDEPATH += ../../MinGW/include/libxml \
               ../../MinGW/include/GL
RESOURCES += \
    Resources.qrc
