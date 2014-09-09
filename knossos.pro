#-------------------------------------------------
#
# Project created by QtCreator 2012-09-05T18:04:31
#
#-------------------------------------------------

QT += core gui opengl network help

TARGET = knossos
TEMPLATE = app
CONFIG += qt c++11 createdoc turbojpeg

SOURCES += *.cpp \
    $$PWD/widgets/*.cpp \
    $$PWD/widgets/commentshortcuts/*.cpp \
    $$PWD/widgets/task/*.cpp \
    $$PWD/widgets/tools/*.cpp \
    $$PWD/widgets/viewportsettings/*.cpp \
    $$PWD/openjpeg/*.c

HEADERS  += *.h \
    $$PWD/widgets/*.h \
    $$PWD/widgets/commentshortcuts/*.h \
    $$PWD/widgets/task/*.h \
    $$PWD/widgets/tools/*.h \
    $$PWD/widgets/viewportsettings/*.h \
    $$PWD/openjpeg/*.h

OTHER_FILES += \
    doc/* \
    doc/images/* \
    LICENSE \
    splash.png \
    default.lut \
    knossos.rc \
    logo.ico \
    ChangeLog_v4.txt

include(scriptengine/scriptengine.pri)

macx:QMAKE_MAC_SDK = macosx10.9
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
macx {
    INCLUDEPATH += /usr/include/Python2.7 \
                   /usr/lib \
                   /usr/include
    LIBS += -framework GLUT \
            -lcurl

    # copy the content of the doc folder to the build-dir
    doc.path = $$OUT_PWD/knossos.app/Contents/MacOS/doc
    doc.files = $$PWD/doc/*
    INSTALLS += doc

    ICON += knossos.icns
}

linux {
    LIBS += -lcurl
    LIBS += -lGLU -lglut
}

win32 {
    #DEFINES += CURL_STATICLIB
    LIBS += -lcurl -lws2_32 -lidn -lz -lintl -liconv

    #DEFINES += FREEGLUT_STATIC
    LIBS += -lglut
    
    DEFINES += QUAZIP_STATIC
    LIBS += -lquazip -lz

    RC_FILE = knossos.rc
}

createdoc {
    createdoctarget.target = $$PWD/doc/collection.qhc
    createdoctarget.depends = $$PWD/doc/collection.qhcp $$PWD/doc/doc.qhp
    createdoctarget.commands = qcollectiongenerator $$PWD/doc/collection.qhcp -o $$PWD/doc/collection.qhc
    QMAKE_EXTRA_TARGETS += createdoctarget
    PRE_TARGETDEPS += $$PWD/doc/collection.qhc
}

turbojpeg {
    DEFINES += KNOSSOS_USE_TURBOJPEG
    mac {
        INCLUDEPATH += /opt/libjpeg-turbo/include
        LIBS += -L/opt/libjpeg-turbo/lib
    }
    win32 {
        INCLUDEPATH += C:/libjpeg-turbo-gcc/include
        LIBS += -LC:/libjpeg-turbo-gcc/lib
    }
    LIBS += -lturbojpeg
}

RESOURCES += Resources.qrc

QMAKE_CXXFLAGS_RELEASE += -flto -O3
