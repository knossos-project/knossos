# Macros for SIP
# ~~~~~~~~~~~~~~
#
# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
# Copyright (c) 2014, Thorben Kroeger <thorben.kroeger@iwr.uni-heidelberg.de> 
#
# Redistribution and use is allowed according to the terms of the BSD license.
#
# SIP website: http://www.riverbankcomputing.co.uk/sip/index.php

FIND_PACKAGE(PythonLibs REQUIRED)

SET(SIP_TAGS)
SET(SIP_CONCAT_PARTS 1)
SET(SIP_DISABLE_FEATURES)

MACRO(ADD_SIP_PYTHON_MODULE MODULE_NAME MODULE_SIP)

    SET(EXTRA_LINK_LIBRARIES ${ARGN})

    STRING(REPLACE "." "/" _x ${MODULE_NAME})
    GET_FILENAME_COMPONENT(_parent_module_path ${_x}  PATH)
    GET_FILENAME_COMPONENT(_child_module_name ${_x} NAME)

    GET_FILENAME_COMPONENT(_module_path ${MODULE_SIP} PATH)

    if(_module_path STREQUAL "")
        set(CMAKE_CURRENT_SIP_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    else(_module_path STREQUAL "")
        set(CMAKE_CURRENT_SIP_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${_module_path}")
    endif(_module_path STREQUAL "")

    GET_FILENAME_COMPONENT(_abs_module_sip ${MODULE_SIP} ABSOLUTE)

    # We give this target a long logical target name.
    # (This is to avoid having the library name clash with any already
    # install library names. If that happens then cmake dependancy
    # tracking get confused.)
    STRING(REPLACE "." "_" _logical_name ${MODULE_NAME})
    SET(_logical_name "python_module_${_logical_name}")

    FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SIP_OUTPUT_DIR})    # Output goes in this dir.

    SET(_sip_includes)
    FOREACH (_inc ${SIP_INCLUDES})
        GET_FILENAME_COMPONENT(_abs_inc ${_inc} ABSOLUTE)
        LIST(APPEND _sip_includes -I ${_abs_inc})
    ENDFOREACH (_inc )

    SET(_sip_tags)
    FOREACH (_tag ${SIP_TAGS})
        LIST(APPEND _sip_tags -t ${_tag})
    ENDFOREACH (_tag)

    SET(_sip_x)
    FOREACH (_x ${SIP_DISABLE_FEATURES})
        LIST(APPEND _sip_x -x ${_x})
    ENDFOREACH (_x ${SIP_DISABLE_FEATURES})

    SET(_message "-DMESSAGE=Generating CPP code for module ${MODULE_NAME}")
    SET(_sip_output_files)
    FOREACH(CONCAT_NUM RANGE 0 ${SIP_CONCAT_PARTS} )
        IF( ${CONCAT_NUM} LESS ${SIP_CONCAT_PARTS} )
            SET(_sip_output_files ${_sip_output_files} ${CMAKE_CURRENT_SIP_OUTPUT_DIR}/sip${_child_module_name}part${CONCAT_NUM}.cpp )
        ENDIF( ${CONCAT_NUM} LESS ${SIP_CONCAT_PARTS} )
    ENDFOREACH(CONCAT_NUM RANGE 0 ${SIP_CONCAT_PARTS} )

    IF(NOT WIN32)
        SET(TOUCH_COMMAND touch)
    ELSE(NOT WIN32)
        SET(TOUCH_COMMAND echo)
        # instead of a touch command, give out the name and append to the files
        # this is basically what the touch command does.
        FOREACH(filename ${_sip_output_files})
            FILE(APPEND filename "")
        ENDFOREACH(filename ${_sip_output_files})
    ENDIF(NOT WIN32)
    
    separate_arguments(SIP_EXTRA_OPTIONS_LIST UNIX_COMMAND "${SIP_EXTRA_OPTIONS}")
    ADD_CUSTOM_COMMAND(
        OUTPUT ${_sip_output_files} 
        COMMAND ${CMAKE_COMMAND} -E echo ${message}
        COMMAND ${TOUCH_COMMAND} ${_sip_output_files} 
        COMMAND ${SIP_EXECUTABLE} ${_sip_tags} ${_sip_x} ${SIP_EXTRA_OPTIONS_LIST} -j ${SIP_CONCAT_PARTS} -c ${CMAKE_CURRENT_SIP_OUTPUT_DIR} ${_sip_includes} ${_abs_module_sip}
        DEPENDS ${_abs_module_sip} ${SIP_EXTRA_FILES_DEPEND}
    )
    # not sure if type MODULE could be uses anywhere, limit to cygwin for now
    IF (CYGWIN)
        ADD_LIBRARY(${_logical_name} MODULE ${_sip_output_files} )
    ELSE (CYGWIN)
        ADD_LIBRARY(${_logical_name} SHARED ${_sip_output_files} )
    ENDIF (CYGWIN)
    TARGET_LINK_LIBRARIES(${_logical_name} ${PYTHON_LIBRARY})
    TARGET_LINK_LIBRARIES(${_logical_name} ${EXTRA_LINK_LIBRARIES})
    SET_TARGET_PROPERTIES(${_logical_name} PROPERTIES PREFIX "" OUTPUT_NAME ${_child_module_name})

    INSTALL(TARGETS ${_logical_name} DESTINATION "${PYTHON_SITE_PACKAGES_INSTALL_DIR}/${_parent_module_path}")
    
ENDMACRO(ADD_SIP_PYTHON_MODULE)
