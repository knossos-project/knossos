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
    openglwidget.cpp


HEADERS  += mainwindow.h \
    ../config.lex \
    ../client.h \
    ../eventHandler.h \
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
    openglwidget.h


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
    blue.jpg
