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

# provides an imported target for the snappy library

find_library(SNAPPY_LIB snappy)
find_path(SNAPPY_INCLUDE snappy.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNAPPY
    REQUIRED_VARS SNAPPY_LIB SNAPPY_INCLUDE
)

if(SNAPPY_FOUND)
    add_library(Snappy::Snappy UNKNOWN IMPORTED)
    set_target_properties(Snappy::Snappy PROPERTIES
        IMPORTED_LOCATION ${SNAPPY_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${SNAPPY_INCLUDE}
    )
endif(SNAPPY_FOUND)
