#ifndef STATE_INFO_H
#define STATE_INFO_H

#include "coordinate.h"
#include "hashtable.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QString>

class stateInfo;
extern stateInfo * state;

//used to be MAX_PATH but conflicted with the official one
#define CSTRING_SIZE 250

#define NUM_MAG_DATASETS 65536

#define DATA_SET 3
#define NO_MAG_CHANGE 0

#define DSP_WHOLE           1
#define DSP_CURRENTCUBE      2
#define DSP_SKEL_VP_HIDE        4
#define DSP_SLICE_VP_HIDE       8
#define DSP_SELECTED_TREES      16
#define DSP_LINES_POINTS        32

// Bytes for an object ID.
#define OBJID_BYTES sizeof(uint64_t)

#define TEXTURE_EDGE_LEN 1024

// UserMove type
enum UserMoveType {USERMOVE_DRILL, USERMOVE_HORIZONTAL, USERMOVE_NEUTRAL};
Q_DECLARE_METATYPE(UserMoveType)

//custom tail recursive constexpr log implementation
//required for the following array declarations/accesses: (because std::log2 isn’t required to be constexpr yet)
//  magPaths, magNames, magBoundaries, Dc2Pointer, Oc2Pointer
//TODO to be removed when the above mentioned variables vanish
//integral return value for castless use in subscripts
template<typename T>
constexpr std::size_t int_log(const T val, const T base = 2, const std::size_t res = 0) {
    return val < base ? res : int_log(val/base, base, res+1);
}

/**
  * @stateInfo
  * @brief stateInfo holds everything we need to know about the current instance of Knossos
  *
  * It gets instantiated in the main method of knossos.cpp and referenced in almost all important files and classes below the #includes with extern  stateInfo
  */
class stateInfo {
public:
    //  Info about the data
    // Use overlay cubes to color the data.
    bool overlay;
    // Tell the loading thread that it should interrupt its work /
    // its sleep and do something new.
    bool loadSignal;
    // Is loader initialized
    bool loaderInitialized;
    // How user movement was generated
    UserMoveType loaderUserMoveType;
    // Direction of user movement in case of drilling,
    // or normal to viewport plane in case of horizontal movement.
    // Left unset in neutral movement.
    Coordinate loaderUserMoveViewportDirection;
    int loaderDecompThreadsNumber;

    bool quitSignal; 

    // If loadSignal is true and breakLoaderSignal is true, make the
    // loading loop break. If quitSignal is also true, the loader thread
    // would quit. Otherwise it would reinitialize.
    // loadSignal == true means the loader
    // has been signalled. If breakLoaderSignal != true, it will go on
    // loading its stuff.
    bool breakLoaderSignal;

    // These signals are used to communicate with the remote.
    bool remoteSignal;

    // Cube hierarchy mode
    bool boergens;

    // Path to the current cube files for the viewer and loader.
      //**rutuja**//
    bool hdf5_found = false;
    std::string hdf5 = "";

    char path[1024];
    char loaderPath[1024];
    // Paths to all available datasets of the 3-D image pyramid
    char magPaths[int_log(NUM_MAG_DATASETS)+1][1024];

    // Current dataset identifier string
    char name[1024];
    char loaderName[1024];
    char magNames[int_log(NUM_MAG_DATASETS)+1][1024];

    // stores the currently active magnification;
    // it is set by magnification = 2^MAGx
    // state->magnification should only be used by the viewer,
    // but its value is copied over to loaderMagnification.
    // This is locked for thread safety.
    // do not change to uint, it causes bugs in the display of higher mag datasets
    int magnification;

    uint compressionRatio;

    uint highestAvailableMag;
    uint lowestAvailableMag;

    // This variable is used only by the loader.
    // It is filled by the viewer and contains
    // log2uint32(state->magnification)
    uint loaderMagnification;

    // Bytes in one datacube: 2^3N
    std::size_t cubeBytes;

    // The edge length of a datacube is 2^N, which makes the size of a
    // datacube in bytes 2^3N which has to be <= 2^32 - 1 (unsigned int).
    // So N cannot be larger than 10.
    // Edge length of one cube in pixels: 2^N
    int cubeEdgeLength;

    // Area of a cube slice in pixels;
    int cubeSliceArea;

    // Supercube edge length in datacubes.
    int M;
    std::size_t cubeSetElements;


    // Bytes in one supercube (This is pretty much the memory
    // footprint of KNOSSOS): M^3 * 2^3M
    std::size_t cubeSetBytes;


    // Edge length of the current data set in data pixels.
    Coordinate boundary;

    // pixel-to-nanometer scale
    floatCoordinate scale;

    // With 2^N being the edge length of a datacube in pixels and
    // M being the edge length of a supercube (the set of all
    // simultaneously loaded datacubes) in datacubes:

// --- Inter-thread communication structures / signals / mutexes, etc. ---

    // Tells the loading thread, that state->path and or state->name changed

    int datasetChangeSignal;

    // Tell the loading thread to wake up.
    QWaitCondition *conditionLoadSignal;

    // Tells another thread that loader has finished
    QWaitCondition *conditionLoaderInitialized;

    // Tell the remote to wake up.
    QWaitCondition *conditionRemoteSignal;

    // Any signalling to the loading thread needs to be protected
    // by this mutex. This is done by sendLoadSignal(), so always
    // use sendLoadSignal() to signal to the loading thread.

    QMutex *protectLoadSignal;

    // Protect slot list during loading
    QMutex *protectLoaderSlots;

    // This should be accessed through sendRemoteSignal() only.
    QMutex *protectRemoteSignal;

    // ANY access to the Dc2Pointer or Oc2Pointer tables has
    // to be locked by this mutex.

    QMutex *protectCube2Pointer;

    QElapsedTimer time; // it is not allowed to modify this object

 //---  Info about the state of KNOSSOS in general. --------

/* Calculate movement trajectory for loading based on how many last single movements */
#define LL_CURRENT_DIRECTIONS_SIZE (20)
    // This gives the current direction whenever userMove is called
    Coordinate currentDirections[LL_CURRENT_DIRECTIONS_SIZE];
    int currentDirectionsIndex;

    // This gives the current position ONLY when the reload
    // boundary has been crossed. Change it through
    // sendLoadSignal() exclusively. It has to be locked by
    // protectLoadSignal.
    Coordinate currentPositionX;
    Coordinate previousPositionX;

    // Cube loader affairs
    int    loadMode;
    char       *loadFtpCachePath;
    char       ftpBasePath[CSTRING_SIZE];
    char       ftpHostName[CSTRING_SIZE];
    char       ftpUsername[CSTRING_SIZE];
    char       ftpPassword[CSTRING_SIZE];
    int    ftpFileTimeout;

    // Dc2Pointer and Oc2Pointer provide a mappings from cube
    // coordinates to pointers to datacubes / overlay cubes loaded
    // into memory.
    // It is a set of key (cube coordinate) / value (pointer) pairs.
    // Whenever we access a datacube in memory, we do so through
    // this structure.
    coord2bytep_map_t Dc2Pointer[int_log(NUM_MAG_DATASETS)+1];
    coord2bytep_map_t Oc2Pointer[int_log(NUM_MAG_DATASETS)+1];

    struct viewerState *viewerState;
    class Viewer *viewer;
    struct skeletonState *skeletonState;
    bool keyD, keyF;
    std::array<float, 3> repeatDirection;
    bool viewerKeyRepeat;
};

#endif//STATE_INFO_H
