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
    openglwidget.cpp \
    eventmodel.cpp \
    client.cpp \
    loader.cpp \
    viewer.cpp \
    remote.cpp \
    skeletonizer.cpp \
    renderer.cpp \
    gui.cpp \
    knossos.cpp \
    coordinate.cpp \
    Hashtable.cpp \
    sleeper.cpp




HEADERS  += mainwindow.h \
    eventmodel.h \
    openglwidget.h \
    client.h \
    loader.h \
    viewer.h \
    remote.h \
    skeletonizer.h \
    renderer.h \
    gui.h \
    knossos-global.h \
    knossos.h \
    ../treeLUT_fallback.h \
    ../treeLUT_fallback.h \
    sleeper.h





FORMS    += mainwindow.ui

OTHER_FILES += \
    ../knossos.layout \
    ../glut32.dll \
    ../iconv.dll \
    ../libfreetype-6.dll \
    ../libSDL_Clipboard.dll \
    ../libxml2.dll \
    ../pthreadVC2.dll \
    ../SDL.dll \
    ../SDL_net.dll \
    ../zlib1.dll \
    ../icon \
    ../LICENSE \
    ../Makefile \
    ../splash \
    ../knossos.depend \
    ../knossos.dev \
    ../default.lut \
    ../gmon.out \
    ../knossos.res \
    ../knossos_private.res \
    ../knossos.rc \
    ../knossos_private.rc \
    ../logo.ico \
    ../ChangeLog.txt \
    ../defaultSettings.xml \
    ../customCursor.xpm \
    ../config.y \

LIBS += -lSDL -lSDL_net -lxml2

INCLUDEPATH += ../../MinGW/include/SDL \
               ../../MinGW/include/libxml \
               ../../MinGW/include/GL
