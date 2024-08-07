# KNOSSOS [![Build Status](https://ci.appveyor.com/api/projects/status/qehrybs5vpvjou28?svg=true&pendingText=building&passingText=OK&failingText=not%20OK)](https://ci.appveyor.com/project/knossos/knossos)

## Background
KNOSSOS is a software tool for the visualization and annotation of 3D image data and was developed for the rapid reconstruction of neural morphology and connectivity.

## Technical
By dynamically loading only data from the surround of the current view point, seamless navigation is not limited to datasets that fit into the available RAM but works with much larger dataset stored in a special format on disk. Currently, KNOSSOS is limited to 8-bit data. In addition to viewing and navigating, KNOSSOS allows efficient manual neurite annotation ('skeletonization').

## Use
KNOSSOS is being used mostly for reconstructing cell morphologies from 3D electron microscopic data generated by Serial Block-Face Electron Microscopy (SBEM), with an occasional application to 2-photon and confocal optical microscopy data.
