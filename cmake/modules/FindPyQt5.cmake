# Find PyQt5
# ~~~~~~~~~~
#
# Based on FindPyQt5.cmake by Simon Edwards.
# Copyright (c) 2007-2008, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
#
# PYQT5_SIP_FLAGS - The SIP flags used to build PyQt.

## FIXME: make generic
#find_package(PythonInterp 3 REQUIRED)
set(PYTHON_EXECUTABLE /usr/bin/python3)

find_path(_sip_include_path
    QtWidgets/qwidget.sip
    HINTS /usr/share/sip/PyQt5
)
set(PYQT5_SIP_INCLUDES ${_sip_include_path}
    CACHE STRING "Path to PyQt SIP includes"
)
message(STATUS "  PyQt5 SIP includes: ${PYQT5_SIP_INCLUDES}")

find_file(_find_pyqt_py FindPyQt5.py PATHS ${CMAKE_MODULE_PATH})
execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_find_pyqt_py} OUTPUT_VARIABLE pyqt_config)
if(pyqt_config)
    string(REGEX REPLACE "pyqt_sip_flags:([^\n;]+).*$" "\\1" PYQT5_SIP_FLAGS ${pyqt_config})
endif(pyqt_config)
message(STATUS "  PyQt5 SIP flags: ${PYQT5_SIP_FLAGS}")

if(PYQT5_SIP_INCLUDES AND PYQT5_SIP_FLAGS)
    set(PYQT5_FOUND TRUE)
endif()

if(PYQT5_FOUND)
    if(NOT PyQt5_FIND_QUIETLY)
        message(STATUS "Found PyQt5, with SIP includes at ${PYQT5_SIP_INCLUDES}") 
        message(STATUS "             and flags ${PYQT5_SIP_FLAGS}") 
    endif()
else()
    if(PyQt5_FIND_REQUIRED)
        message(STATUS "Could not find PyQt5")
        message(STATUS "Try setting the PYQT5_SIP_INLCUDES")
        message(STATUS "for example: /usr/share/sip/PyQt5")
        message(FATAL_ERROR "Aborting because PyQt5 could not be found")
    endif()
endif()
