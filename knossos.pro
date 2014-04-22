
#-------------------------------------------------
#
# Project created by QtCreator 2012-09-05T18:04:31
#
#-------------------------------------------------

QT       += core gui opengl network xml help

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = knossos
TEMPLATE = app
CONFIG += warn_off #c++11
#CONFIG += console


SOURCES += openjpeg/cio.c \
    openjpeg/bio.c \
    openjpeg/color.c \
    openjpeg/convert.c \
    openjpeg/dwt.c \
    openjpeg/event.c \
    openjpeg/function_list.c \
    openjpeg/image.c \
    openjpeg/index.c \
    openjpeg/invert.c \
    openjpeg/j2k.c \
    openjpeg/jp2.c \
    openjpeg/mct.c \
    openjpeg/mqc.c \
    openjpeg/openjpeg.c \
    openjpeg/opj_clock.c \
    openjpeg/opj_decompress.c \
    openjpeg/opj_getopt.c \
    openjpeg/pi.c \
    openjpeg/raw.c \
    openjpeg/t1.c \
    openjpeg/t2.c \
    openjpeg/tcd.c \
    openjpeg/tgt.c \
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
    functions.cpp \
    qsort.cpp \
    ftp.cpp \
    stateInfo.cpp \
    Color4F.cpp \
    NodeListElement.cpp \
    SegmentListElement.cpp \
    TreeListElement.cpp \
    treeLUT_fallback.cpp \
    sha256.cpp \
    widgets/viewport.cpp \    
    widgets/mainwindow.cpp \
    widgets/console.cpp \
    widgets/tracingtimewidget.cpp \
    widgets/commentswidget.cpp \
    widgets/commentshortcuts/commentshortcutstab.cpp \
    widgets/commentshortcuts/commentshighlightingtab.cpp \
    widgets/zoomandmultireswidget.cpp \
    widgets/datasavingwidget.cpp \
    widgets/navigationwidget.cpp \
    widgets/viewportsettingswidget.cpp \
    widgets/viewportsettings/vpsliceplaneviewportwidget.cpp \
    widgets/viewportsettings/vpskeletonviewportwidget.cpp \
    widgets/viewportsettings/vpgeneraltabwidget.cpp \
    widgets/synchronizationwidget.cpp \
    widgets/splashscreenwidget.cpp \
    widgets/datasetpropertywidget.cpp \
    widgets/task/task.cpp \
    widgets/task/taskloginwidget.cpp \
    widgets/task/taskmanagementwidget.cpp \
    widgets/task/taskmanagementmaintab.cpp \
    widgets/task/taskmanagementdetailstab.cpp \
    widgets/widgetcontainer.cpp \
    widgets/commentshortcuts/commentsnodecommentstab.cpp \
    widgets/documentationwidget.cpp \
    widgets/annotationwidget.cpp \
    widgets/tools/toolscommandstab.cpp \
    widgets/tools/toolstreeviewtab.cpp \
    widgets/tools/nodetable.cpp \
    widgets/tools/treetable.cpp \
    Mesh.cpp \
    widgets/gui.cpp



PRECOMPILED_HEADERS += openjpeg/tgt.h \
    openjpeg/tcd.h \
    openjpeg/t2.h \
    openjpeg/t1_luts.h \
    openjpeg/t1.h \
    openjpeg/raw.h \
    openjpeg/pi.h \
    openjpeg/opj_stdint.h \
    openjpeg/opj_malloc.h \
    openjpeg/opj_inttypes.h \
    openjpeg/opj_intmath.h \
    openjpeg/opj_includes.h \
    openjpeg/opj_getopt.h \
    openjpeg/opj_config_private.h \
    openjpeg/opj_config.h \
    openjpeg/opj_clock.h \
    openjpeg/opj_apps_config.h \
    openjpeg/openjpeg.h \
    openjpeg/mqc.h \
    openjpeg/mct.h \
    openjpeg/jp2.h \
    openjpeg/j2k.h \
    openjpeg/invert.h \
    openjpeg/indexbox_manager.h \
    openjpeg/index.h \
    openjpeg/image.h \
    openjpeg/function_list.h \
    openjpeg/format_defs.h \
    openjpeg/event.h \
    openjpeg/dwt.h \
    openjpeg/convert.h \
    openjpeg/color.h \
    openjpeg/cio.h \
    openjpeg/cidx_manager.h \
    openjpeg/bio.h

HEADERS  += widgets/mainwindow.h \
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
    functions.h \
    ftp.h \
    sha256.h \
    widgets/GuiConstants.h \
    widgets/viewport.h \
    widgets/console.h \
    widgets/tracingtimewidget.h \
    widgets/commentswidget.h \
    widgets/commentshortcuts/commentshortcutstab.h \
    widgets/commentshortcuts/commentshighlightingtab.h \
    widgets/zoomandmultireswidget.h \
    widgets/datasavingwidget.h \
    widgets/navigationwidget.h \
    widgets/viewportsettingswidget.h \
    widgets/viewportsettings/vpsliceplaneviewportwidget.h \
    widgets/viewportsettings/vpskeletonviewportwidget.h \
    widgets/viewportsettings/vpgeneraltabwidget.h \
    widgets/synchronizationwidget.h \
    widgets/splashscreenwidget.h \
    widgets/datasetpropertywidget.h \
    widgets/task/taskloginwidget.h \
    widgets/task/taskmanagementwidget.h \
    widgets/task/taskmanagementmaintab.h \
    widgets/task/taskmanagementdetailstab.h \
    widgets/commentshortcuts/commentsnodecommentstab.h \    
    widgets/widgetcontainer.h \
    widgets/documentationwidget.h \
    widgets/annotationwidget.h \
    widgets/tools/toolscommandstab.h \
    widgets/tools/toolstreeviewtab.h \
    widgets/tools/nodetable.h \
    widgets/tools/treetable.h \
    widgets/gui.h


FORMS    += mainwindow.ui

OTHER_FILES += \
    doc/* \
    doc/images/* \
    LICENSE \
    Makefile \
    splash.png \
    default.lut \
    knossos.res \
    knossos_private.res \
    knossos.rc \
    knossos_private.rc \
    logo.ico \
    ChangeLog.txt \
    ChangeLog_v4.txt \
    style.qss \

include(scriptengine/scriptengine.pri)
#include(test/test.pri)

exists(.svn) {
    #stringify (svnversion ouput maybe not integer)
    SVNREV = \\\"$$system(svnversion)\\\"
    DEFINES += REVISION=$$SVNREV
    message(svn revision: $$SVNREV)
}
exists(.git) {
    COMMIT = $$system(git log -1 --pretty=format:%H)
    GITREV = $$system(git svn find-rev $$COMMIT)
    DEFINES += REVISION=$$GITREV
    message(git svn revision: $$GITREV)
}

macx:QMAKE_MAC_SDK = macosx10.9
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
macx {
    INCLUDEPATH += /usr/lib/ \
                   /usr/include \
    LIBS += -framework GLUT \
            -lcurl

    # copy the content of the doc folder to the build-dir
    doc.path = $$OUT_PWD/knossos.app/Contents/MacOS/doc
    doc.files = $$PWD/doc/*
    INSTALLS += doc

    ICON += knossos.icns
}

linux {
    LIBS += -lGL \
            -lGLU \
            -lglut \
            -lcurl \
            -L/usr/lib/i386-linux-gnu/mesa/lGL \

    INCLUDEPATH += /usr/include/GL/ \
            /usr/local/include/ \


    # copy the content of the doc folder to the build-dir
    doc.path = $$OUT_PWD/doc
    doc.files = $$PWD/doc/*

    scripts.path = $$OUT_PWD/python
    scripts.files = $$PWD/python/*

    INSTALLS += doc python

}

win32 {
    CONFIG(debug, debug|release) {
        DEBUG_EXT = _d
    } else {
        DEBUG_EXT =
    }

    LIBS += -lcurl.dll \
            #-lglew32 \
            -lglut32 \
            -lwsock32

    RC_FILE = knossos.rc
}


RESOURCES += \
    Resources.qrc


QMAKE_CXXFLAGS += -std=gnu++0x #-std=c++0x
QMAKE_CXXFLAGS_RELEASE += -O3
