/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "coordinate.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QString>
#include <QWaitCondition>

#include <unordered_map>
#include <vector>

class stateInfo;
inline stateInfo * state{nullptr};

#define NUM_MAG_DATASETS 65536

using coord2bytep_map_t = std::vector<std::vector<std::unordered_map<CoordOfCube, void *>>>;

inline void * cubeQuery(const coord2bytep_map_t &h, const std::size_t layerId, const std::size_t magindex, const CoordOfCube &c) {
    try {
        return h.at(layerId).at(magindex).at(c);
    } catch (const std::out_of_range &) {
        return nullptr;
    }
}

// Bytes for an object ID.
#define OBJID_BYTES sizeof(uint64_t)

/**
  * @stateInfo
  * @brief stateInfo holds everything we need to know about the current instance of Knossos
  *
  * It gets instantiated in the main method of knossos.cpp and referenced in almost all important files and classes below the #includes with extern  stateInfo
  */
class stateInfo {
public:
    //  Info about the data
    bool gpuSlicer = false;

    bool quitSignal{false};

    // Bytes in one datacube: 2^3N
    std::size_t cubeBytes;

    // Supercube edge length in datacubes.
    int M;
    std::size_t cubeSetElements;

    // Bytes in one supercube (This is pretty much the memory
    // footprint of KNOSSOS): M^3 * 2^3M
    std::size_t cubeSetBytes;

    // With 2^N being the edge length of a datacube in pixels and
    // M being the edge length of a supercube (the set of all
    // simultaneously loaded datacubes) in datacubes:

// --- Inter-thread communication structures / signals / mutexes, etc. ---

    // ANY access to the Dc2Pointer or Oc2Pointer tables has
    // to be locked by this mutex.
    QMutex protectCube2Pointer;

 //---  Info about the state of KNOSSOS in general. --------

    // Dc2Pointer and Oc2Pointer provide a mappings from cube
    // coordinates to pointers to datacubes / overlay cubes loaded
    // into memory.
    // It is a set of key (cube coordinate) / value (pointer) pairs.
    // Whenever we access a datacube in memory, we do so through
    // this structure.
    coord2bytep_map_t cube2Pointer;

    struct ViewerState * viewerState;
    class MainWindow * mainWindow{nullptr};
    class Viewer * viewer;
    class Scripting * scripting;
    class SignalRelay * signalRelay;
    struct SkeletonState * skeletonState;
};
