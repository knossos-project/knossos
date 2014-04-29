HEADERS +=  \
    scriptengine/decorators/treelistdecorator.h \
    scriptengine/decorators/nodelistdecorator.h \
    scriptengine/decorators/mainwindowdecorator.h \
    scriptengine/decorators/colordecorator.h \
    scriptengine/decorators/segmentlistdecorator.h \
    scriptengine/decorators/transformdecorator.h \
    scriptengine/decorators/floatcoordinatedecorator.h \
    scriptengine/decorators/meshdecorator.h \
    scriptengine/decorators/coordinatedecorator.h \
    scriptengine/decorators/pointdecorator.h \
    scriptengine/highlighter.h \
    scriptengine/geometry/transform.h \
    scriptengine/geometry/shape.h \
    scriptengine/geometry/point.h \
    scriptengine/geometry/render.h \
    scriptengine/proxies/skeletonproxy.h \
    scriptengine/scripting.h


SOURCES += \
    scriptengine/decorators/treelistdecorator.cpp \
    scriptengine/decorators/nodelistdecorator.cpp \
    scriptengine/decorators/mainwindowdecorator.cpp \
    scriptengine/decorators/pointdecorator.cpp \
    scriptengine/decorators/floatcoordinatedecorator.cpp \
    scriptengine/decorators/coordinatedecorator.cpp \
    scriptengine/decorators/transformdecorator.cpp \
    scriptengine/decorators/segmentlistdecorator.cpp \
    scriptengine/decorators/colordecorator.cpp \
    scriptengine/decorators/meshdecorator.cpp \
    scriptengine/highlighter.cpp \
    scriptengine/geometry/transform.cpp \
    scriptengine/geometry/shape.cpp \
    scriptengine/geometry/point.cpp \
    scriptengine/geometry/render.cpp \
    scriptengine/proxies/skeletonproxy.cpp \
    scriptengine/scripting.cpp


OTHER_FILES += \
    scriptengine/python/converter.py \
    scriptengine/python/examples/pathes.py \
    scriptengine/python/user/custom_graphics_view.py \
    scriptengine/python/user/images/* \



linux {
    LIBS += -lPythonQt \
            -lPythonQt_QtAll \
            -L/usr/lib/python2.7 -lpython2.7

    INCLUDEPATH += $(QTDIR)/include \
                   /usr/include/python2.7/ \


    # copy the content of the python folder to the build-dir
    scripts.path = $$OUT_PWD/knossos.app/Contents/MacOS/python \
    scripts.files = scriptengine/python/* \
    INSTALLS += scripts \
}

win32 {
    CONFIG(debug, debug|release) {
        DEBUG_EXT = _d
    } else {
        DEBUG_EXT =
    }

    LIBS += -lpythonQt$${DEBUG_EXT} \
            -lpythonQt_QtAll$${DEBUG_EXT} \


    INCLUDEPATH += C:\Qt\Qt5.1.0\Tools\mingw48_32\opt\include\python2.7 \
                   C:\Qt\Qt5.1.0\Tools\mingw48_32\opt\include\
                   C:\Qt\Qt5.1.0\5.1.0\mingw48_32\include

    # copy the content of the python folder to the build-dir
    scripts.path = $$OUT_PWD/python
    scripts.files = scriptengine/python/*
    INSTALLS += scripts

}
