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


    For further information, visit http://www.knossostool.org
    or contact knossos-team@mpimf-heidelberg.mpg.de
]]

#specify extraordinary locations for convenience here (forward slashes essential)
#cmake searches for headers and libraries both directly and inside include/lib folders below
set(CMAKE_PREFIX_PATH
    "${CMAKE_PREFIX_PATH}"#append

# For Mac development: required to find packages from Homebrew
    "/usr/local/opt/qt5/bin"
)

# find static qt libs (default msys2 location), MINGW_PREFIX is /mingw??
if(WIN32 AND NOT BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "x64 static build")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{MINGW_PREFIX}/qt5-static/")
elseif(WIN32 AND NOT BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "x32 static build")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{MINGW_PREFIX}/qt5-static/")
endif()

# find system python dll
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SystemRoot}/System32")
elseif(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SystemRoot}/SysWOW64")
endif()
