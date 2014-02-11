#
#   This file is a part of KNOSSOS.
# 
#   (C) Copyright 2007-2011
#   Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
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

import math, re, glob, os

class KnossosConfig():
    def __init__(self):
        self.xReg = re.compile('_x(\d*)')
        self.yReg = re.compile('_y(\d*)')
        self.zReg = re.compile('_z(\d*)')
        self.magReg = re.compile('_mag(\d*)([\\/]$|$)')
        self.expNameReg = re.compile(r'([^\\/]*)_x\d{4}_y\d{4}_z\d{4}.raw')

        self.namePattern = re.compile('experiment name \"(.*)\";')
        self.scaleXPattern = re.compile('scale x (.*\..*);')
        self.scaleYPattern = re.compile('scale y (.*\..*);')
        self.scaleZPattern = re.compile('scale z (.*\..*);')
        self.boundaryXPattern = re.compile('boundary x (.*);')
        self.boundaryYPattern = re.compile('boundary y (.*);')
        self.boundaryZPattern = re.compile('boundary z (.*);')
        self.magnificationPattern = re.compile('magnification (.*);')

    def generate(self, path):
        # try to read magnification factor from directory name
        try:
            magnification = self.magReg.search(path).groups()[0]
        except AttributeError:
            magnification = None

        # read files in current directory
        files = glob.glob(str(path) + "/x????/y????/z????/*.raw");

        try:
            filesize = float(os.stat(files[0]).st_size)
        except (IndexError, OSError):
            raise DataError("Error determining file size")
            return

        edgeLength = int(round(math.pow(filesize, 1. / 3.), 0))

        try:
            name = self.expNameReg.search(files[0]).groups()[0]
        except AttributeError:
            raise DataError("Error matching file name")
            return

        xlen_datacubes, ylen_datacubes, zlen_datacubes = 0, 0, 0
        for file in files:
            try:
                x = int(self.xReg.search(file).groups()[0])
                y = int(self.yReg.search(file).groups()[0])
                z = int(self.zReg.search(file).groups()[0])
            except AttributeError:
                raise DataError("Error matching file name")
                return

            if x > xlen_datacubes and x < 9999:
                xlen_datacubes  = x
            if y > ylen_datacubes and y < 9999:
                ylen_datacubes  = y
            if z > zlen_datacubes and z < 9999:
                zlen_datacubes  = z

        xlen_px = (xlen_datacubes + 1) * edgeLength
        ylen_px = (ylen_datacubes + 1) * edgeLength
        zlen_px = (zlen_datacubes + 1) * edgeLength

        return name, (xlen_px, ylen_px, zlen_px), magnification

    def read(self, path):
        kconfigpath = os.path.abspath(path + "/knossos.conf")

        try:
            kFile = open(kconfigpath)
        except IOError:
            try:
                name, boundaries, magnification = self.generate(path)
            except DataError:
                raise
                return

            configInfo = [True,
                          name,
                          path,
                          (None, None, None),
                          boundaries,
                          magnification,
                          ""]

            return configInfo
        else:         
            configText = kFile.read()
            kFile.close()

        namePatternResult = self.namePattern.search(configText)
        scaleXPatternResult = self.scaleXPattern.search(configText)
        scaleYPatternResult = self.scaleYPattern.search(configText)
        scaleZPatternResult = self.scaleZPattern.search(configText)
        boundaryXPatternResult = self.boundaryXPattern.search(configText)
        boundaryYPatternResult = self.boundaryYPattern.search(configText)
        boundaryZPatternResult = self.boundaryZPattern.search(configText)
        magnificationPatternResult = self.magnificationPattern.search(configText)

        try:
            name = namePatternResult.groups()[0]
        except (AttributeError, ValueError):
            name = ""
        try:
            scaleX = float(scaleXPatternResult.groups()[0])
        except (AttributeError, ValueError):
            scaleX = None
        try:
            scaleY = float(scaleYPatternResult.groups()[0])
        except (AttributeError, ValueError):
            scaleY = None
        try:
            scaleZ = float(scaleZPatternResult.groups()[0])
        except (AttributeError, ValueError):
            scaleZ = None
        try:
            boundaryX = int(boundaryXPatternResult.groups()[0])
        except (AttributeError, ValueError):
            boundaryX = 0
        try:
            boundaryY = int(boundaryYPatternResult.groups()[0])
        except (AttributeError, ValueError):
            boundaryY = 0
        try:
            boundaryZ = int(boundaryZPatternResult.groups()[0])
        except (AttributeError, ValueError):
            boundaryZ = 0
        try:
            magnification = int(magnificationPatternResult.groups()[0])
        except (AttributeError, ValueError):
            magnification = None

        # [is incomplete?, name, path, scales, boundary, magnification, original config file contents]
        configInfo = [False,
                      name,
                      path,
                      (scaleX, scaleY, scaleZ),
                      (boundaryX, boundaryY, boundaryZ),
                      magnification,
                      configText]

        return configInfo

    def write(self, configInfo):
        try:
            source = configInfo["Source"]
            name = configInfo["Name"]
            scales = configInfo["Scales"]
            boundaries = configInfo["Boundaries"]
            path = configInfo["Path"]
            magnification = configInfo["Magnification"]
        except KeyError:
            return False
            
        if self.namePattern.search(source):
            source = self.namePattern.sub("experiment name \"%s\";" % name, source)
        else:
            source = source + "\nexperiment name \"%s\";" % name

        if self.scaleXPattern.search(source):
            source = self.scaleXPattern.sub("scale x %s;" % str(float(scales[0])), source)
        else:
            source = source + "\nscale x %s;" % str(float(scales[0]))

        if self.scaleYPattern.search(source):
            source = self.scaleYPattern.sub("scale y %s;" % str(float(scales[1])), source)
        else:
            source = source + "\nscale y %s;" % str(float(scales[1]))
        
        if self.scaleZPattern.search(source):
            source = self.scaleZPattern.sub("scale z %s;" % str(float(scales[2])), source)
        else:
            source = source + "\nscale z %s;" % str(float(scales[2]))

        if self.boundaryXPattern.search(source):
            source = self.boundaryXPattern.sub("boundary x %d;" % boundaries[0], source)
        else:
            source = source + "\nboundary x %d;" % boundaries[0]

        if self.boundaryYPattern.search(source):
            source = self.boundaryYPattern.sub("boundary y %d;" % boundaries[1], source)
        else:
            source = source + "\nboundary y %d;" % boundaries[1]

        if self.boundaryZPattern.search(source):
            source = self.boundaryZPattern.sub("boundary z %d;" % boundaries[2], source)
        else:
            source = source + "\nboundary z %d;" % boundaries[2]

        if self.magnificationPattern.search(source):
            source = self.magnificationPattern.sub("magnification %d;\n" % magnification, source)
        else:
            source = source + "\nmagnification %d;" % magnification

        confpath = os.path.abspath(path + "/knossos.conf")

        try:
            kFile = open(confpath, "w")
            kFile.write(source)
            kFile.close()
        except IOError:
            return False

    def check(self, config):
        try:
            if config["Name"] is not ""       \
                and config["Path"] is not ""  \
                and config["Scales"][0]       \
                and config["Scales"][1]       \
                and config["Scales"][2]       \
                and config["Magnification"]:
            
                return False
            else:
                return True
        except KeyError:
            return True

class DataError(Exception):
    def __init__(self, errorstring):
        self.errorstring = errorstring

    def __str__(self):
        return repr(self.errorstring)
