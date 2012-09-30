#-------------------------------------------------
#
# Project created by QtCreator 2012-09-05T18:04:31
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = knossos
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    openglwidget.cpp \
    eventmodel.cpp \
    ../client.c \
    ../coordinate.c \
    ../hash.c \
    ../knossos.c \
    ../lex.yy.c \
    ../loader.c \
    ../remote.c \
    ../renderer.c \
    ../skeletonizer.c \
    ../viewer.c \
    ../y.tab.c \



HEADERS  += mainwindow.h \
    ../config.lex \
    ../client.h \
    ../hash.h \
    ../knossos-global.h \
    ../knossos_private.h \
    ../knossos.h \
    ../lex.yy.h \
    ../loader.h \
    ../remote.h \
    ../renderer.h \
    ../skeletonizer.h \
    ../treeLUT_fallback.h \
    ../viewer.h \
    ../y.tab.h \
    openglwidget.h \
    eventmodel.h


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
    blue.jpg \
    ../../knossos-skeletonizer/zlib1.dll \
    ../../knossos-skeletonizer/SDL_net.dll \
    ../../knossos-skeletonizer/SDL.dll \
    ../../knossos-skeletonizer/pthreadVC2.dll \
    ../../knossos-skeletonizer/libxml2.dll \
    ../../knossos-skeletonizer/libSDL_Clipboard.dll \
    ../../knossos-skeletonizer/libfreetype-6.dll \
    ../../knossos-skeletonizer/iconv.dll \
    ../../knossos-skeletonizer/glut32.dll

LIBS += -lSDL -lxml2

INCLUDEPATH += ../../MinGW/include/SDL \
               ../../MinGW/include/libxml \
               ../usr/include/

