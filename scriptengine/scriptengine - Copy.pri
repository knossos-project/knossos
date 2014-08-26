HEADERS +=  \
    scriptengine/highlighter.h \
    scriptengine/geometry/transform.h \
    scriptengine/geometry/shape.h \
    scriptengine/geometry/point.h \
    scriptengine/geometry/render.h \
    scriptengine/scripting.h \
    scriptengine/proxies/skeletonproxy.h \
    scriptengine/proxies/pythonproxy.h


SOURCES += \
    scriptengine/highlighter.cpp \
    scriptengine/geometry/transform.cpp \
    scriptengine/geometry/shape.cpp \
    scriptengine/geometry/point.cpp \
    scriptengine/geometry/render.cpp \
    scriptengine/scripting.cpp \
    scriptengine/proxies/skeletonproxy.cpp \
    scriptengine/proxies/pythonproxy.cpp


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
    scripts.path = $$OUT_PWD/python \
    scripts.files = $$PWD/python/* \
    INSTALLS += scripts \
}

win32 {
    CONFIG(debug, debug|release) {
        INCLUDEPATH += c:\oren\_python27_debug_install\include
        LIBS += -Lc:\oren\_python27_debug_install\lib
        DEBUG_EXT = _d
    } else {
        INCLUDEPATH += c:\oren\_python27_install\include
        LIBS += -Lc:\oren\_python27_install\lib
        DEBUG_EXT =
    }
    INCLUDEPATH += C:\Python27\include

    LIBS += -lPythonQt5$${DEBUG_EXT}.dll \
            -lPythonQt5_QtAll$${DEBUG_EXT}.dll \

    # copy the content of the python folder to the build-dir
    #scripts.path = $$OUT_PWD/python
    #scripts.files = scriptengine/python/*
    #INSTALLS += scripts

}
