#[[
    This file is a part of KNOSSOS.

    (C) Copyright 2007-2016
    Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.

    KNOSSOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 of
    the License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


    For further information, visit https://knossostool.org
    or contact knossos-team@mpimf-heidelberg.mpg.de
]]

# provides an imported target for the quazip library

find_package(ZLIB REQUIRED)

find_library(QUAZIP_LIBRARY NAMES quazip quazip5 PATH_SUFFIXES "QuaZip")
find_path(QUAZIP_INCLUDE_DIR quazip.h PATH_SUFFIXES quazip quazip5)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QUAZIP
    REQUIRED_VARS QUAZIP_LIBRARY QUAZIP_INCLUDE_DIR
)

if(QUAZIP_FOUND)
    add_library(QuaZip::QuaZip UNKNOWN IMPORTED)
    set_target_properties(QuaZip::QuaZip PROPERTIES
        IMPORTED_LOCATION ${QUAZIP_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${QUAZIP_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
    )
    if(NOT ${QUAZIP_LIBRARY} MATCHES ".so" AND NOT ${QUAZIP_LIBRARY} MATCHES ".dll")
    set_target_properties(QuaZip::QuaZip PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "QUAZIP_STATIC"
    )
    endif()
endif(QUAZIP_FOUND)
