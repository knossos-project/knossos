/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

/**
  * @file knossos-global.h
  * @brief This file contains global #includes, #defines, makros and struct definitions
  * The elements are used in almost all other files.
  */

#ifndef KNOSSOS_GLOBAL_H
#define KNOSSOS_GLOBAL_H

#include "coordinate.h"
#include "widgets/navigationwidget.h"

/** The includes in this header has to be part of a qt module and only C header. Otherwise the Python C API can´t use it  */
#include <curl/curl.h>

#include <array>
#include <cmath>
#include <unordered_map>

#include <QtOpenGL/qgl.h>
#include <QtCore/QElapsedTimer>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtNetwork/qhostinfo.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qset.h>
#include <QtCore/qdatetime.h>

#define KVERSION "4.1 Pre Alpha"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif
#define PI 3.141592653589793238462643383279

#define TEXTURE_EDGE_LEN 1024

#define NUM_MAG_DATASETS 65536

#if QT_POINTER_SIZE == 4
#define PTRSIZEUINT uint
#elif QT_POINTER_SIZE == 8
#define PTRSIZEUINT uint64_t
#endif

// The edge length of a datacube is 2^N, which makes the size of a
// datacube in bytes 2^3N which has to be <= 2^32 - 1 (unsigned int).
// So N cannot be larger than 10.

// Bytes for an object ID.
#define OBJID_BYTES sizeof(uint64_t)

//	For the hashtable.
#define HT_SUCCESS  1
#define HT_FAILURE  0
#define LL_SUCCESS  1
#define LL_FAILURE  0
#define LL_BEFORE   0
#define LL_AFTER    2

 //	For the viewer.
#define	SLICE_XY	0
#define SLICE_XZ	1
#define	SLICE_YZ	2

#define NUM_VP 4 //number of viewports
enum ViewportType {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ, VIEWPORT_SKELETON, VIEWPORT_UNDEFINED, VIEWPORT_ARBITRARY};
Q_DECLARE_METATYPE(ViewportType)
/* VIEWPORT_ORTHO has the same value as the XY VP, this is a feature, not a bug.
This is used for LOD rendering, since all ortho VPs have the (about) the same screenPxPerDataPx
values. The XY vp always used. */
const auto VIEWPORT_ORTHO = VIEWPORT_XY;

// UserMove type
enum UserMoveType {USERMOVE_DRILL, USERMOVE_HORIZONTAL, USERMOVE_NEUTRAL};
Q_DECLARE_METATYPE(UserMoveType)

// default position of xy viewport and default viewport size
#define DEFAULT_VP_MARGIN 5
#define DEFAULT_VP_SIZE 350

// skeleton vp orientation
#define SKELVP_XY_VIEW 0
#define SKELVP_XZ_VIEW 1
#define SKELVP_YZ_VIEW 2
#define SKELVP_R90 3
#define SKELVP_R180 4
#define SKELVP_RESET 5

// MAG is a bit unintiutive here: a lower MAG means in KNOSSOS that a
// a pixel of the lower MAG dataset has a higher resolution, i.e. 10 nm
// pixel size instead of 20 nm
#define MAG_DOWN 1
#define MAG_UP 2
#define DATA_SET 3
#define NO_MAG_CHANGE 0

#define ON_CLICK_DRAG    0
#define ON_CLICK_RECENTER 1

// vp zoom max < vp zoom min, because vp zoom level translates to displayed edgeLength.
// close zoom -> smaller displayed edge length
#define VPZOOMMAX  0.02000
#define VPZOOMMIN   1.0
#define SKELZOOMMAX 0.4999
#define SKELZOOMMIN 0.0

#define NUM_COMMSUBSTR 5 //number of comment substrings for conditional comment node coloring/radii

//  For the Lookup tables
 #define RGB_LUTSIZE  768
 #define RGBA_LUTSIZE 1024
 #define MAX_COLORVAL  255.

//  For the skeletonizer
#define SEGMENT_FORWARD 1
#define SEGMENT_BACKWARD 2

#define DSP_WHOLE           1
#define DSP_CURRENTCUBE      2
#define DSP_SKEL_VP_HIDE        4
#define DSP_SLICE_VP_HIDE       8
#define DSP_SELECTED_TREES      16
#define DSP_LINES_POINTS        32

#define CATCH_RADIUS            10
#define JMP_THRESHOLD           500 //for moving to next/prev node

#define ROTATIONSTATEXY    0
#define ROTATIONSTATEXZ    1
#define ROTATIONSTATEYZ    2
#define ROTATIONSTATERESET 3

#define SLOW 1000
#define FAST 10

class stateInfo;
extern stateInfo * state;

class floatCoordinate;
class color4F;
class treeListElement;
class nodeListElement;
class segmentListElement;
class mesh;

//Structures and custom types
typedef uint8_t Byte;

//custom tail recursive constexpr log implementation
//required for the following array declarations/accesses: (because std::log2 isn’t required to be constexpr yet)
//  magPaths, magNames, magBoundaries, Dc2Pointer, Oc2Pointer
//TODO to be removed when the above mentioned variables vanish
//integral return value for castless use in subscripts
template<typename T>
constexpr std::size_t int_log(const T val, const T base = 2, const std::size_t res = 0) {
    return val < base ? res : int_log(val/base, base, res+1);
}

class color4F {
public:
    color4F();
    color4F(float r, float g, float b, float a);
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat a;
};

// This structure makes up the linked list that is used to store the data for
// the hash table. The linked is circular, but has one entry element that is
// defined by the pointer in the hash table structure below.
//
// * coordinate is of type coordinate (as defined in coordinate.h) and i used
//   as the key in the hash table.
// * datacube is a pointer to the datacube that is to be associated with the
//   coordinate coordinate.
// * next is a pointer to another structure of the same type.
//   someElement->next->previous = someElement should always hold true.
// * previous should behave in analogy to next
// * ht_next either is the same as next or is NULL. It is used to manage the
//   chains to resolve collisions in the hash table. ht_next only points to the
//   next element if that element has the same key as the current element and is
//   NULL else.

struct C2D_Element {
        Coordinate coordinate;
        Byte *datacube;
        C2D_Element *previous;
        C2D_Element *next;
        C2D_Element *ht_next;
};

// This structure defines a hash table. It is passed to various functions
// along with some other parameters to perform actions on a specific hash
// table.
// The structure is generated and filled by the function ht_new (see below).
//
// * listEntry is a dummy element in the list that is used only to enter the
//   list. Its Datacube-pointer is set to NULL and its Coordinates to (-1, -1,
//   -1). As the coordinate system begins at (0, 0, 0), that's invalid.
// * tablesize stores the size of the table and is always a power of two.
// * table is a pointer to a table of pointers to elements in the linked list
//   (of which listEntry is one).

typedef struct Hashtable{
    C2D_Element *listEntry;
    uint tablesize;
    C2D_Element **table;

    // Create a new hashtable.
    // tablesize specifies the size of the hash-to-data-table and should be 1.25
    // to 1.5 times the number of elements the table is expected to store.
    // If successful, the function returns a pointer to a Hashtable structure.
    static Hashtable *ht_new(uint tablesize);

    // Delete the whole hashtable and linked list and release all the memory
    // hashtable specifies which hashtable to delete.
    // The return value is LL_SUCCESS or LL_FAILURE
    static uint ht_rmtable(Hashtable *hashtable);

    // Return the value associated with a key.
    // key is the key to look for.
    // hashtable is the hashtable to look in.
    // On success, a pointer to Byte (a Datacube) is returned, HT_FAILURE else.
    static Byte *ht_get(Hashtable *hashtable, Coordinate key);
    // Internal implementation of ht_get, only return the hash element where data is stored.
    // This should also be used when you actually want to change something in the element
    // if it exists.
    static C2D_Element *ht_get_element(Hashtable *hashtable, Coordinate key);

    // Insert an element.
    static uint ht_put(Hashtable *hashtable, Coordinate key, Byte *value);

    // Delete an element
    static uint ht_del(Hashtable *hashtable, Coordinate key);
    // Compute the union of h1 and h2 and put it in target.
    static uint ht_union(Hashtable *target, Hashtable *h1, Hashtable *h2);

    static uint nextpow2(uint a);
    static uint lastpow2(uint a);

} Hashtable;

struct stack {
    uint elementsOnStack;
    void **elements;
    int stackpointer;
    int size;
};

/**
  * @stateInfo
  * @brief stateInfo holds everything we need to know about the current instance of Knossos
  *
  * It gets instantiated in the main method of knossos.cpp and referenced in almost all important files and classes below the #includes with extern  stateInfo
  */

class stateInfo : public QObject {
    Q_OBJECT
public:
    //  Info about the data
    // Use overlay cubes to color the data.
    bool overlay;
    // Tell the loading thread that it should interrupt its work /
    // its sleep and do something new.
    bool loadSignal;
    // Is loader currently busy
    bool loaderBusy;
    // How user movement was generated
    Byte loaderUserMoveType;
    // Direction of user movement in case of drilling,
    // or normal to viewport plane in case of horizontal movement.
    // Left unset in neutral movement.
    Coordinate loaderUserMoveViewportDirection;
    // Should loader load real data or just dummy do-nothing
    bool loaderDummy;
    int loaderDecompThreadsNumber;

    // If loadSignal is true and quitSignal is true, make the
    // loading thread quit. loadSignal == true means the loader
    // has been signalled. If quitSignal != true, it will go on
    // loading its stuff.
    bool quitSignal;

    // These signals are used to communicate with the remote.
    bool remoteSignal;

    // Cube hierarchy mode
    bool boergens;

    // Path to the current cube files for the viewer and loader.
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
    QWaitCondition *conditionLoadFinished;

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
    char       ftpBasePath[MAX_PATH];
    char       ftpHostName[MAX_PATH];
    char       ftpUsername[MAX_PATH];
    char       ftpPassword[MAX_PATH];
    int    ftpFileTimeout;

    // Dc2Pointer and Oc2Pointer provide a mappings from cube
    // coordinates to pointers to datacubes / overlay cubes loaded
    // into memory.
    // It is a set of key (cube coordinate) / value (pointer) pairs.
    // Whenever we access a datacube in memory, we do so through
    // this structure.
    Hashtable *Dc2Pointer[int_log(NUM_MAG_DATASETS)+1];
    Hashtable *Oc2Pointer[int_log(NUM_MAG_DATASETS)+1];

    struct viewerState *viewerState;
    class Viewer *viewer;
    struct skeletonState *skeletonState;
    struct taskState *taskState;
    bool keyD, keyF;
    std::array<float, 3> repeatDirection;
    bool viewerKeyRepeat;
};

struct httpResponse {
    char *content;
    size_t length;
};

struct taskState {
    QString host;
    QString cookieFile;
    QString taskFile;
    QString taskName;

    static bool httpGET(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static bool httpPOST(char *url, char *postdata, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static bool httpDELETE(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static bool httpFileGET(char *url, char *postdata, httpResponse *response, struct httpResponse *header, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static size_t writeHttpResponse(void *ptr, size_t size, size_t nmemb, struct httpResponse *s);
    static size_t readFile(char *ptr, size_t size, size_t nmemb, void *stream);
    static int copyInfoFromHeader(char *dest, struct httpResponse *header, const char *info);
    static void removeCookie();
    static QString CSRFToken();
    static QString getCategory();
    static QString getTask();
};

/**
  * @struct viewportTexture
  * @brief TODO
  */

struct viewportTexture {
    //Handles for OpenGl
    uint texHandle;
    uint overlayHandle;

    //The absPx coordinate of the upper left corner of the texture actually stored in *texture
    Coordinate leftUpperPxInAbsPx;
    uint edgeLengthDc;
    uint edgeLengthPx;

    //These variables specifiy the area inside the textures which are used
    //for data storage. Storage always starts at texture pixels (0,0).
    uint usedTexLengthDc;
    uint usedTexLengthPx;

    //These variables specifiy the lengths inside the texture that are currently displayed.
    //Their values depend on the zoom level and the data voxel dimensions (because of aspect
    //ratio correction). Units are texture coordinates.
    float displayedEdgeLengthX;
    float displayedEdgeLengthY;

    float texUnitsPerDataPx;

    //Texture coordinates
    float texLUx, texLUy, texLLx, texLLy, texRUx, texRUy, texRLx, texRLy;
    //Coordinates of crosshair inside VP
    float xOffset, yOffset;

    // Current zoom level. 1: no zoom; near 0: maximum zoom.
    float zoomLevel;

};

/**
  * @struct guiConfig
  * @brief TODO
  *
  */
struct guiConfig {
    char titleString[2048];

    // Current position of the user crosshair,
    //starting at 1 instead 0. This is shown to the user,
    //KNOSSOS works internally with 0 start indexing.
    Coordinate oneShiftedCurrPos;
    Coordinate activeNodeCoord;

    QString lockComment;
    char *commentBuffer;
    char *commentSearchBuffer;
    char *treeCommentBuffer;

    int useLastActNodeRadiusAsDefault;
    float actNodeRadius;

    // dataset navigation settings win buffer variables
    uint stepsPerSec;
    uint recenteringTime;
    uint dropFrames;

    char *comment1;
    char *comment2;
    char *comment3;
    char *comment4;
    char *comment5;

    // substrings for comment node highlighting
    QStringList *commentSubstr;
    //char **commentSubstr;
    // colors of color-dropdown in comment node highlighting
    char **commentColors;
};

/**
  * @struct vpConfig
  * @brief Contains attributes for widget size, screen pixels per data pixels,
  *        as well as flags about user interaction with the widget
  */
struct vpConfig {
    // s*v1 + t*v2 = px
    floatCoordinate n;
    floatCoordinate v1; // vector in x direction
    floatCoordinate v2; // vector in y direction
    floatCoordinate leftUpperPxInAbsPx_float;
    floatCoordinate leftUpperDataPxOnScreen_float;
    int s_max;
    int t_max;
    int x_offset;
    int y_offset;

    Byte* viewPortData;

    struct viewportTexture texture;

    //The absPx coordinate of the upper left corner pixel of the currently on screen displayed data
    Coordinate leftUpperDataPxOnScreen;

    //This is a bit confusing..the screen coordinate system has always
    //x on the horizontal and y on the verical axis, but the displayed
    //data pixels can have a different axis. Keep this in mind.
    //These values depend on texUnitsPerDataPx (in struct viewportTexture),
    //the current zoom value and the data pixel voxel dimensions.
    float screenPxXPerDataPx;
    float screenPxYPerDataPx;

    // These are computed from screenPxPerDataPx and are used to convert
    // screen coordinates into coordinates in the system of the data at the
    // original magnification. Node coordinates are stored that way to make
    // knossos instances working with different magnifications of the same data
    // work together well.
    float screenPxXPerOrigMagUnit; //unused? jk 14.5.12
    float screenPxYPerOrigMagUnit; //unused? jk 14.5.12

    float displayedlengthInNmX;
    float displayedlengthInNmY;

    ViewportType type; // type e {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ, VIEWPORT_SKELETON, VIEWPORT_ARBITRARY}
    uint id; // id e {VP_UPPERLEFT, VP_LOWERLEFT, VP_UPPERRIGHT, VP_LOWERRIGHT}
    // CORRECT THIS COMMENT TODO BUG
    //lower left corner of viewport in screen pixel coords (max: window borders)
    //we use here the lower left corner, because the openGL intrinsic coordinate system
    //is defined over the lower left window corner. All operations inside the viewports
    //use a coordinate system with lowest coordinates in the upper left corner.
    Coordinate upperLeftCorner;
    //edge length in screen pixel coordinates; only squarish VPs are allowed

    uint edgeLength;

    //Flag that indicates that the user is moving the VP inside the window. 0: off, 1: moving
    Byte VPmoves;
    //Flag that indicates that the user is resizing the VP inside the window. 0: off, 1: resizing
    Byte VPresizes;
    //Flag that indicates that another VP is covering this VP
    Byte covered;
    //Flag that indicates that the user utilizes the mouse for operations inside this VP
    //necessary because the mouse can be outside the VP while the user still wants e.g. to drag the panel
    Byte motionTracking;

    struct nodeListElement *draggedNode;

    /* Stores the current view frustum planes */
    float frustum[6][4];

    GLuint displayList;

    //Variables that store the mouse "move path length". This is necessary, because not every mouse move pixel
    //would result in a data pixel movement
    float userMouseSlideX;
    float userMouseSlideY;
};

/**
  * @struct viewerState
  * @brief TODO
  */
struct viewerState {
    vpConfig *vpConfigs;
    Byte *texData;
    Byte *overlayData;
    Byte *defaultTexData;
    Byte *defaultOverlayData;
    uint numberViewports;
    uint splash;
    bool viewerReady;
    GLuint splashTexture;
    //Flag to indicate user movement
    bool userMove;
    int highlightVp;
    int vpKeyDirection[3];

    //Min distance to currently centered data cube for rendering of spatial skeleton structure.
    //Unit: data cubes.
    int zoomCube;

    // don't jump between mags on zooming
    bool datasetMagLock;

    float depthCutOff;

    // Current position of the user crosshair.
    //   Given in pixel coordinates of the current local dataset (whatever magnification
    //   is currently loaded.)
    Coordinate currentPosition;

    uint recenteringTime;
    uint recenteringTimeOrth;
    bool walkOrth;

    //SDL_Surface *screen;

    //Keyboard repeat rate
    uint stepsPerSec;
    GLint filterType;
    int multisamplingOnOff;
    int lightOnOff;

    // Draw the colored lines that highlight the orthogonal VP intersections with each other.
    bool drawVPCrosshairs;
    // flag to indicate if user has pulled/is pulling a selection square in a viewport, which should be displayed
    int nodeSelectSquareVpId;
    std::pair<Coordinate, Coordinate> nodeSelectionSquare;

    //Show height/width-labels inside VPs
    bool showVPLabels;

    bool selectModeFlag;

    uint dropFrames;
    uint walkFrames;

    float voxelDimX;
    float voxelDimY;
    float voxelXYRatio;
    float voxelDimZ;
    //YZ can't be different to XZ because of the intrinsic properties of the SBF-SEM.
    float voxelXYtoZRatio;

    // allowed are: ON_CLICK_RECENTER 1, ON_CLICK_DRAG 0
    uint clickReaction;

    struct guiConfig *gui;

    int luminanceBias;
    int luminanceRangeDelta;

    GLuint datasetColortable[3][256];
    GLuint datasetAdjustmentTable[3][256];
    bool datasetColortableOn;
    bool datasetAdjustmentOn;
    GLuint neutralDatasetTable[3][256];

    bool treeLutSet;
    bool treeColortableOn;
    float treeColortable[RGB_LUTSIZE];
    float treeAdjustmentTable[RGB_LUTSIZE];
    float defaultTreeTable[RGB_LUTSIZE];

    bool uniqueColorMode;

    // Advanced Tracing Modes Stuff
    navigationMode autoTracingMode;
    int autoTracingDelay;
    int autoTracingSteps;

    float cumDistRenderThres;

    bool defaultVPSizeAndPos;
    QDateTime lastIdleTimeCall;
    uint renderInterval;
};

struct commentListElement {
    struct commentListElement *next;
    struct commentListElement *previous;

    char *content;

    struct nodeListElement *node;
};

class treeListElement {
public:
    treeListElement();
    treeListElement(int treeID, QString comment, color4F color);
    treeListElement(int treeID, QString comment, float r, float g, float b, float a);

    treeListElement *next;
    treeListElement *previous;
    nodeListElement *firstNode;

    int treeID;
    color4F color;
    bool selected;
    int colorSetManually;
    std::size_t size;

    char comment[8192];
    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();

};

class nodeListElement {
public:
   nodeListElement();
   nodeListElement(int nodeID, int x, int y, int z, int parentID, float radius, int inVp, int inMag, int time);

    nodeListElement *next;
    nodeListElement *previous;

    segmentListElement *firstSegment;

    treeListElement *correspondingTree;

    float radius;

    //can be VIEWPORT_XY, ...
    Byte createdInVp;
    Byte createdInMag;
    int timestamp;

    commentListElement *comment;

    // counts forward AND backward segments!!!
    int numSegs;

    // circumsphere radius - max. of length of all segments and node radius.
    //Used for frustum culling
    float circRadius;
    uint nodeID;
    Coordinate position;
    bool isBranchNode;
    bool selected;

    QList<segmentListElement *> *getSegments();
};

class segmentListElement {
public:
    segmentListElement() {}
    segmentListElement *next;
    segmentListElement *previous;

    //Contains the reference to the segment inside the target node
    segmentListElement *reverseSegment;

    // 1 signals forward segment 2 signals backwards segment.
    // Use SEGMENT_FORWARD and SEGMENT_BACKWARD.
    Byte flag;
    float length;
    //char *comment;

    //Those coordinates are not the same as the coordinates of the source / target nodes
    //when a segment crosses the skeleton DC boundaries. Then these coordinates
    //lie at the skeleton DC borders. This applies only, when we use the segment inside
    //a skeleton DC, not inside of a tree list.
    //Coordinate pos1;
    //Coordinate pos2;

    nodeListElement *source;
    nodeListElement *target;
};

class mesh {
public:
    mesh();
    mesh(int mode); /* GL_POINTS, GL_TRIANGLES, etc. */
    floatCoordinate *vertices;
    floatCoordinate *normals;
    color4F *colors;

    /* useful for flexible mesh manipulations */
    uint vertsBuffSize;
    uint normsBuffSize;
    uint colsBuffSize;
    /* indicates last used element in corresponding buffer */
    uint vertsIndex;
    uint normsIndex;
    uint colsIndex;
    uint mode;
    uint size;
};

/**
  * @struct skeletonState
  */
struct skeletonState {
    uint skeletonRevision;

    //    skeletonTime is the time spent on the current skeleton in all previous
    //    instances of knossos that worked with the skeleton.
    //    skeletonTimeCorrection is the time that has to be subtracted from
    //    state->time->elapsed() to yield the number of milliseconds the current skeleton
    //    was loaded in the current knossos instance.

    bool unsavedChanges;
    int skeletonTime;
    int skeletonTimeCorrection;

    int idleTimeSession;
    int idleTime;
    int idleTimeNow;
    int idleTimeLast;

    bool simpleTracing;
    struct treeListElement *firstTree;
    struct treeListElement *activeTree;
    struct nodeListElement *activeNode;

    std::vector<treeListElement *> selectedTrees;
    std::vector<nodeListElement *> selectedNodes;

    struct commentListElement *currentComment;
    char *commentBuffer;
    char *searchStrBuffer;

    struct stack *branchStack;

    std::unordered_map<uint, nodeListElement *> nodesByNodeID;

    uint volBoundary;

    uint totalComments;
    uint totalBranchpoints;

    bool userCommentColoringOn;
    uint commentNodeRadiusOn;

    bool lockPositions;
    bool positionLocked;
    char onCommentLock[1024];
    Coordinate lockedPosition;
    long unsigned int lockRadius;

    float rotdx;
    float rotdy;
    int rotationcounter;

    int definedSkeletonVpView;

    float translateX, translateY;

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];

    // Stores the angles of the cube in the SkeletonVP
    float rotationState[16];
    // The next three flags cause recompilation of the above specified display lists.

    //true, if all display lists must be updated
    bool skeletonChanged;

    uint displayMode;

    float segRadiusToNodeRadius;
    int overrideNodeRadiusBool;
    float overrideNodeRadiusVal;

    int highlightActiveTree;
    int showIntersections;
    int rotateAroundActiveNode;
    int showXYplane;
    int showXZplane;
    int showYZplane;
    int showNodeIDs;
    bool autoFilenameIncrementBool;

    int treeElements;
    int totalNodeElements;
    int totalSegmentElements;

    uint greatestNodeID;
    int greatestTreeID;

    color4F commentColors[NUM_COMMSUBSTR];
    float commentNodeRadii[NUM_COMMSUBSTR];
    nodeListElement *selectedCommentNode;

    //If true, loadSkeleton merges the current skeleton with the provided
    int mergeOnLoadFlag;

    uint lastSaveTicks;
    bool autoSaveBool;
    uint autoSaveInterval;

    float defaultNodeRadius;

    // Current zoom level. 0: no zoom; near 1: maximum zoom.
    float zoomLevel;

    // temporary vertex buffers that are available for rendering, get cleared
    // every frame */
    mesh lineVertBuffer; /* ONLY for lines */
    mesh pointVertBuffer; /* ONLY for points */

    bool branchpointUnresolved;

    // This is for a workaround around agar bug #171
    bool askingPopBranchConfirmation;
    char skeletonCreatedInVersion[32];
    char skeletonLastSavedInVersion[32];
};

/* global functions */

/* Macros */
#define SET_COLOR(color, rc, gc, bc, ac) \
        { \
        (color).r = (rc); \
        (color).g = (gc); \
        (color).b = (bc); \
        (color).a = (ac); \
        }

#define ABS(x) (((x) >= 0) ? (x) : -(x))
#define SQR(x) ((x)*(x))

#define INNER_MULT_VECTOR(v) ((v).x * (v).y * (v).z)

#define CALC_VECTOR_NORM(v) \
    ( \
        sqrt(SQR((v).x) + SQR((v).y) + SQR((v).z)) \
    )

#define CALC_DOT_PRODUCT(a, b) \
    ( \
        ((a).x * (b).x) + ((a).y * (b).y) + ((a).z * (b).z) \
    )
#define CALC_POINT_DISTANCE_FROM_PLANE(point, plane) \
    ( \
        ABS(CALC_DOT_PRODUCT((point), (plane))) / CALC_VECTOR_NORM((plane)) \
    )

// This is used by the hash function. It rotates the bits by n to the left. It
// works analogously to the 8086 assembly instruction ROL and should actually
// compile to that instruction on a decent compiler (confirmed for gcc).
// ONLY FOR UNSIGNED 32-BIT INT, of course.
#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// Compute a % b with b being a power of two. This works because a % b will
// preserve all the bits less significant than the most significant bit
// of b (if b is a power of two).
#define MODULO_POW2(a, b)   (a) & ((b) - 1)

#endif
