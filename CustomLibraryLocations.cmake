#[[
    This file is a part of KNOSSOS.

    (C) Copyright 2007-2018
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

#specify extraordinary locations for convenience here (forward slashes essential)
#cmake searches for headers and libraries both directly and inside include/lib folders below
list(APPEND CMAKE_PREFIX_PATH
# For Mac development: required to find packages from Homebrew
    "/usr/local/opt/qt5/bin"
)
