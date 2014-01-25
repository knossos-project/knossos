#
#   This file is a part of KNOSSOS.
# 
#   (C) Copyright 2007-2011
#   Max-Planck-Gesellschaft zur Förderung der Wissenschaften e.V.
# 
#   KNOSSOS is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 of
#   the License as published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#
#
#   For further information, visit http://www.knossostool.org or contact
#       Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
#       Fabian.Svara@mpimf-heidelberg.mpg.de
#

import re, glob, os, sys, shutil

xReg = re.compile('_x(\d*)')
yReg = re.compile('_y(\d*)')
zReg = re.compile('_z(\d*)')

files = glob.glob(os.getcwd() + "/*.raw")
files = files + glob.glob(os.getcwd() + "/*.overlay")

for file in files:
    try:
        x = int(xReg.search(file).groups()[0])
        y = int(yReg.search(file).groups()[0])
        z = int(zReg.search(file).groups()[0])
    except AttributeError:
        print("Incorrectly formatted .raw file: " + file)
        continue

    newDir = os.path.abspath('x%04d/y%04d/z%04d/' % (x, y, z))
    try:
        os.makedirs(os.path.normpath(newDir))
    except OSError:
        pass

    print(os.path.normpath(newDir))
    shutil.move(file, os.path.normpath(newDir + '/'))
