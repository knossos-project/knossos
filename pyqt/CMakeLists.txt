find_package(SIP REQUIRED)
include_directories(${SIP_INCLUDE_DIR})
find_package(PyQt5 REQUIRED)

set(SIP_INCLUDES "${PYQT5_SIP_INCLUDES}")
set(SIP_EXTRA_OPTIONS "${PYQT5_SIP_FLAGS}")
include(SIPMacros)

add_library(testwidget testwidget.cpp)
qt5_use_modules(testwidget Widgets)

add_sip_python_module(pyknossos
    testwidget.sip
    testwidget
)

get_property(location TARGET python_module_pyknossos PROPERTY LOCATION)
add_custom_command(TARGET python_module_pyknossos
    POST_BUILD        
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  
        ${location}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

