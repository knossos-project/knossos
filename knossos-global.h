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

#include <QtOpenGL>
#include <QWaitCondition>
#include <QMutex>
#include <QtNetwork>
#include <QSet>

#define KVERSION "3.4"

#define FAIL    -1

#define MIN_N   1

#define PI 3.141592653589793238462643383279

#define TEXTURE_EDGE_LEN 1024

#define NUM_MAG_DATASETS 65536

#if QT_POINTER_SIZE == 4
#define PTRSIZEINT int
#elif QT_POINTER_SIZE == 8
#define PTRSIZEINT int64_t
#endif

// The edge length of a datacube is 2^N, which makes the size of a
// datacube in bytes 2^3N which has to be <= 2^32 - 1 (unsigned int).
// So N cannot be larger than 10.
#define MAX_N   10
#define MIN_M   1
#define MAX_M   10
#define MIN_BOUNDARY    1
#define MAX_BOUNDARY    9999

// Bytes for an object ID.
#define OBJID_BYTES  3

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
#define VIEWPORT_XY	0
#define VIEWPORT_XZ	1
#define VIEWPORT_YZ	2
#define VIEWPORT_SKELETON 3
#define VIEWPORT_UNDEFINED 4
/* VIEWPORT_ORTHO has the same value as the XY VP, this is a feature, not a bug.
This is used for LOD rendering, since all ortho VPs have the (about) the same screenPxPerDataPx
values. The XY vp always used. */
#define VIEWPORT_ORTHO 0

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 3

#define USER_YES 1
#define USER_NO 0
#define USER_UNDEFINED 2

// MAG is a bit unintiutive here: a lower MAG means in KNOSSOS that a
// a pixel of the lower MAG dataset has a higher resolution, i.e. 10 nm
// pixel size instead of 20 nm
#define MAG_DOWN 1
#define MAG_UP 2
#define NO_MAG_CHANGE 0

#define CUBE_DATA       0
#define CUBE_OVERLAY    1

// Those are used to determine if the coordinate change should be passed on to
// clients by the server.
#define SILENT_COORDINATE_CHANGE 0
#define TELL_COORDINATE_CHANGE 1

#define ON_CLICK_RECENTER 1
#define ON_CLICK_DRAG    0
#define ON_CLICK_SELECT   2
#define SKELETONIZER      2

#define VPZOOMMAX  0.02
#define VPZOOMMIN   1.0
#define SKELZOOMMAX 0.4999
#define SKELZOOMMIN 0.0

#define NUM_COMMSHORTCUTS 5 //number of shortcut places for comments
#define NUM_COMMSUBSTR 5 //number of comment substrings for conditional comment node coloring/radii

//  For the Lookup tables

 #define RGB_LUTSIZE  768
 #define RGBA_LUTSIZE 1024
 #define MAX_COLORVAL  255.

//  For the GUI.

#define MAX_RECENT_FILES 10

// 	For the remote.

#define MAX_BUFFER_SIZE 52428800 // 50 MiB

// USEREVENTS
#define USEREVENT_JUMP 5
#define USEREVENT_MOVE 6
#define USEREVENT_REDRAW 7
#define USEREVENT_NOAUTOSAVE 8
#define USEREVENT_REALQUIT 9

#define REMOTE_TRAJECTORY 5
#define REMOTE_FOLLOW 6
#define REMOTE_SYNCHRONIZE 7
#define REMOTE_RECENTERING 8

//  For the client / server protocol

// CHANGE_MANUAL is the revision count used to signal a skeleton change on behalf of the
// user to lockSkeleton().
#define CHANGE_MANUAL 0
#define CHANGE_NOSYNC -1

#define KIKI_REPEAT 0
#define KIKI_CONNECT 1
#define KIKI_DISCONNECT 2
#define KIKI_HI 3
#define KIKI_HIBACK 4
#define KIKI_BYE 5
#define KIKI_ADVERTISE 6
#define KIKI_WITHDRAW 7
#define KIKI_POSITION 8
#define KIKI_ADDCOMMENT 9
#define KIKI_EDITCOMMENT 10
#define KIKI_DELCOMMENT 11
#define KIKI_ADDNODE 12
#define KIKI_EDITNODE 13
#define KIKI_DELNODE 14
#define KIKI_ADDSEGMENT 15
#define KIKI_DELSEGMENT 16
#define KIKI_ADDTREE 17
#define KIKI_DELTREE 18
#define KIKI_MERGETREE 19
#define KIKI_SPLIT_CC 20
#define KIKI_PUSHBRANCH 21
#define KIKI_POPBRANCH 22
#define KIKI_CLEARSKELETON 23
#define KIKI_SETACTIVENODE 24
#define KIKI_SETSKELETONMODE 25
#define KIKI_SAVEMASTER 26
#define KIKI_SKELETONFILENAME 27
#define KIKI_ADDTREECOMMENT 28

//  For custom key bindings

#define ACTION_NONE 0

//  For the skeletonizer

#define SKELETONIZER_ON_CLICK_ADD_NODE 0
#define SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE 1
#define SKELETONIZER_ON_CLICK_DEL_NODE 2
#define SKELETONIZER_ON_CLICK_DROP_NODE 3
#define SEGMENT_FORWARD 1
#define SEGMENT_BACKWARD 2

//#define DISPLAY_WHOLE_SKELETON 0
//#define DISPLAY_CURRENTCUBE_SKELETON 1
//#define DISPLAY_ACTIVETREE_SKELETON 2
//#define DISPLAY_NOTHING_SKELETON 3
//#define DISPLAY_ONLYSLICEPLANE_SKELETON 4
//#define DISPLAY_SEGS_AS_LINES 5
//#define DISPLAY_LINES_POINTS_ONLY 6

#define NODE_VISITED 1
#define NODE_PRISTINE 0

#define DSP_SKEL_VP_WHOLE       1
#define DSP_SKEL_VP_CURRENTCUBE 2
#define DSP_SKEL_VP_HIDE        4
#define DSP_SLICE_VP_HIDE       8
#define DSP_ACTIVETREE          16
#define DSP_LINES_POINTS        32

#define CATCH_RADIUS            10
#define JMP_THRESHOLD           500 //for moving to next/prev node

#define CURRENT_MAG_COORDINATES     0
#define ORIGINAL_MAG_COORDINATES    1

#define AUTOTRACING_NORMAL  0
#define AUTOTRACING_VIEWPORT    1
#define AUTOTRACING_TRACING 2
#define AUTOTRACING_MIRROR  3

#define ROTATIONSTATERESET 0
#define ROTATIONSTATEXY 1
#define ROTATIONSTATEXZ 3
#define ROTATIONSTATEYZ 2

#define AUTOTRACING_MODE_NORMAL 0
#define AUTOTRACING_MODE_ADDITIONAL_VIEWPORT_DIRECTION_MOVE 1
#define AUTOTRACING_MODE_ADDITIONAL_TRACING_DIRECTION_MOVE 2
#define AUTOTRACING_MODE_ADDITIONAL_MIRRORED_MOVE 3

/* command types */

//next line should be changeable for user later...
#define CMD_MAXSTEPS 10 // Must not be 0!!

#define CMD_DELTREE 1 //delete tree
#define CMD_ADDTREE 2 //add tree
#define CMD_SPLITTREE 3 //split tree
#define CMD_MERGETREE 4 //merge tree
#define CMD_CHGTREECOL 5 //change tree color
#define CMD_CHGACTIVETREE 6 //change active tree

#define CMD_DELNODE 7 //delete node
#define CMD_ADDNODE 8 //add node
#define CMD_LINKNODE 9 //add segment
#define CMD_UNLINKNODE 10 //delete segment
#define CMD_PUSHBRANCH 11 //add branchpoint
#define CMD_POPBRANCH 12 //pop branchpoint
#define CMD_CHGACTIVENODE 13 //change active node

#define CMD_ADDCOMMENT 14 //add comment
#define CMD_CHGCOMMENT 15 //change comment
#define CMD_DELCOMMENT 16 //delete comment

//Structures and custom types
typedef uint8_t Byte;

typedef struct {
        float x;
        float y;
        float z;
} floatCoordinate;

struct Coordinate{
    int x;
    int y;
    int z;
    static Coordinate Px2DcCoord(Coordinate pxCoordinate);
    static bool transCoordinate(Coordinate *outCoordinate, int x, int y, int z, floatCoordinate scale, Coordinate offset);
    static Coordinate *transNetCoordinate(uint id, uint x, uint y, uint z);
    static Coordinate *parseRawCoordinateString(char *string);

};

typedef struct {
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat a;
} color4F;

struct _CubeSlot {
        Byte *cube;
        struct _CubeSlot *next;
        struct _CubeSlot *previous;
};

typedef struct _CubeSlot CubeSlot;

struct _CubeSlotList {
        CubeSlot *firstSlot;
        int elements;
};

typedef struct _CubeSlotList CubeSlotList;

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

struct _C2D_Element {
        Coordinate coordinate;
        Byte *datacube;
        struct _C2D_Element *previous;
        struct _C2D_Element *next;
        struct _C2D_Element *ht_next;
};

typedef struct _C2D_Element C2D_Element;

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

    // Insert an element.
    static uint ht_put(Hashtable *hashtable, Coordinate key, Byte *value);

    // Delete an element
    static uint ht_del(Hashtable *hashtable, Coordinate key);
    // Compute the union of h1 and h2 and put it in target.
    static uint ht_union(Hashtable *target, Hashtable *h1, Hashtable *h2);

    static uint nextpow2(uint a);
    static uint lastpow2(uint a);
} Hashtable;

// This is used for a linked list of datacube slices that have to be processed for a given viewport.
// A backlog is generated when we want to retrieve a specific slice from a dc but that dc
// is unavailable at that time.
struct vpBacklogElement {
	Byte *slice;
    Coordinate cube;
	// I guess those aren't really necessary.
	uint x_px;
	uint y_px;
    uint dcOffset;
    uint cubeType;
    struct vpBacklogElement *next;
    struct vpBacklogElement *previous;
};

struct vpBacklog {
    struct vpBacklogElement *entry;
    uint elements;
};

// This is used for a (counting) linked list of vpConfigs that have to be handled
// (their textures put together from datacube slices)
struct vpListElement {
    struct vpConfig *vpConfig;
    struct vpBacklog *backlog;
    struct vpListElement *next;
    struct vpListElement *previous;
};

struct vpList {
    struct vpListElement *entry;
    uint elements;
};

struct stack {
    uint elementsOnStack;
    void **elements;
    int stackpointer;
    int size;
};

struct dynArray {
    void **elements;
    int end;
    int firstSize;
};

struct assignment {
    char *lval;
    char *rval;
};

/**
  * @struct stateInfo
  * @brief stateInfo holds everything we need to know about the current instance of Knossos
  *
  * It gets instantiated in the main method of knossos.cpp and referenced in almost all important files and classes below the #includes with extern struct stateInfo
  */

struct stateInfo {

    //  Info about the data
    // Use overlay cubes to color the data.
    bool overlay;
    // Tell the loading thread that it should interrupt its work /
    // its sleep and do something new.
    bool loadSignal;

    // If loadSignal is TRUE and quitSignal is TRUE, make the
    // loading thread quit. loadSignal == TRUE means the loader
    // has been signalled. If quitSignal != TRUE, it will go on
    // loading its stuff.
    bool quitSignal;

    // These signals are used to communicate with the remote.
    bool remoteSignal;

    // Same for the client. The client threading code is basically
    // copy-pasted from the remote.
    bool clientSignal;

    // Cube hierarchy mode
    bool boergens;

    // Path to the current cube files for the viewer and loader.
    char path[1024];
    char loaderPath[1024];
    // Paths to all available datasets of the 3-D image pyramid
    char magPaths[NUM_MAG_DATASETS][1024];

    // Current dataset identifier string
    char name[1024];
    char loaderName[1024];
    char magNames[NUM_MAG_DATASETS][1024];

    char datasetBaseExpName[1024];


    // stores the currently active magnification;
    // it is set by magnification = 2^MAGx
    // state->magnification should only be used by the viewer,
    // but its value is copied over to loaderMagnification.
    // This is locked for thread safety.
    uint magnification;

    uint highestAvailableMag;
    uint lowestAvailableMag;

    // This variable is used only by the loader.
    // It is filled by the viewer and contains
    // log2uint32(state->magnification)
    uint loaderMagnification;

    // Bytes in one datacube: 2^3N
    uint cubeBytes;

    // Edge length of one cube in pixels: 2^N
    int cubeEdgeLength;

    // Area of a cube slice in pixels;
    int cubeSliceArea;

    // Supercube edge length in datacubes.
    int M;
    uint cubeSetElements;


    // Bytes in one supercube (This is pretty much the memory
    // footprint of KNOSSOS): M^3 * 2^3M
    uint cubeSetBytes;


    // Edge length of the current data set in data pixels.
    Coordinate boundary;
    //Coordinate loaderBoundary;
    Coordinate *magBoundaries[NUM_MAG_DATASETS];

    // pixel-to-nanometer scale
    floatCoordinate scale;

    // offset for synchronization between datasets
    Coordinate offset;

    // With 2^N being the edge length of a datacube in pixels and
    // M being the edge length of a supercube (the set of all
    // simultaneously loaded datacubes) in datacubes:

// --- Inter-thread communication structures / signals / mutexes, etc. ---

    // Tells the loading thread, that state->path and or state->name changed

    int datasetChangeSignal;

    int maxTrajectories;

    // Tell the loading thread to wake up.

    QWaitCondition *conditionLoadSignal;

    // Tell the remote to wake up.
    QWaitCondition *conditionRemoteSignal;

    QWaitCondition *conditionClientSignal;

    // Any signalling to the loading thread needs to be protected
    // by this mutex. This is done by sendLoadSignal(), so always
    // use sendLoadSignal() to signal to the loading thread.

    QMutex *protectLoadSignal;

    // This should be accessed through sendRemoteSignal() only.

    QMutex *protectRemoteSignal;

    // Access through sendClientSignal()

    QMutex *protectClientSignal;

    // ANY access to the Dc2Pointer or Oc2Pointer tables has
    // to be locked by this mutex.

    QMutex *protectCube2Pointer;

    //  Protect the network output buffer and the network peers list


    QMutex *protectOutBuffer;

    QMutex *protectPeerList;

    //  Protect changes to the skeleton for network synchronization.

    QMutex *protectSkeleton;

    QTime time; // it is not allowed to modify this object

 //---  Info about the state of KNOSSOS in general. --------

    // This gives the current position ONLY when the reload
    // boundary has been crossed. Change it through
    // sendLoadSignal() exclusively. It has to be locked by
    // protectLoadSignal.
    Coordinate currentPositionX;

    // Dc2Pointer and Oc2Pointer provide a mappings from cube
    // coordinates to pointers to datacubes / overlay cubes loaded
    // into memory.
    // It is a set of key (cube coordinate) / value (pointer) pairs.
    // Whenever we access a datacube in memory, we do so through
    // this structure.
    Hashtable *Dc2Pointer[NUM_MAG_DATASETS];
    Hashtable *Oc2Pointer[NUM_MAG_DATASETS];

    struct viewerState *viewerState;
    struct clientState *clientState;
    struct skeletonState *skeletonState;
    struct trajectory *trajectories;
};

struct trajectory {
		char name[64];
		char *source;
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
    char settingsFile[2048];
    char titleString[2048];

    char recentFiles[MAX_RECENT_FILES][4096];
   // AG_MenuItem *appMenuRoot;

    // Current position of the user crosshair,
    //starting at 1 instead 0. This is shown to the user,
    //KNOSSOS works internally with 0 start indexing.
    Coordinate oneShiftedCurrPos;
    Coordinate activeNodeCoord;

    int yesNoReturn;

    // tools win buffer variables
    int activeNodeID;
    int activeTreeID;
    int totalNodes;
    int totalTrees;
    int numBranchPoints;
    char *commentBuffer;
    char *commentSearchBuffer;
    char *treeCommentBuffer;

    int mergeTreesID1;
    int mergeTreesID2;

    int linkActiveWithNode;
    int useLastActNodeRadiusAsDefault;
    float actNodeRadius;

    color4F actTreeColor;

    // File dialog widget variables and buffers
  //  AG_FileType *fileTypeNml;

    char skeletonDirectory[2048];
    char datasetLUTDirectory[2048];
    char datasetImgJDirectory[2048];
    char treeImgJDirectory[2048];
    char treeLUTDirectory[2048];
    char customPrefsDirectory[2048];
    char treeLUTFile[2048];
    char datasetLUTFile[2048];

    // file saving settings win buffer variables
    uint autoSaveInterval; // in minutes
    int onSaveAutoIncrement;

    // viewport pref window
    int enableLinearFiltering;
    int radioRenderingModel;
    int enableOrthoSkelOverlay;
    int radioSkeletonDisplayMode;

    // synchronization settings win buffer variables

    // dataset navigation settings win buffer variables
    uint stepsPerSec;
    uint recenteringTime;
    uint dropFrames;

    // skeleton statistics win buffer variables

    int agInputWdgtFocused;

   // AG_Window *agWin;


   // AG_Window *navOptWin;
  //  AG_Window *dispOptWin;
  //  AG_Window *syncOptWin;
  //  AG_Label *syncOptLabel;
  //  AG_Window *saveOptWin;
  //  AG_Window *renderingOptWin;
  //  AG_Window *spatLockOptWin;
  //  AG_Window *volTraceOptWin;
  //  AG_Window *dataSetStatsWin;
  //  AG_Window *viewportPrefWin;
  //  AG_Window *zoomingWin;
  //  AG_Window *tracingTimeWin;
  //  AG_Window *commentsWin;
 //   AG_Window *setDynRangeWin;
//	AG_Window *coordBarWin;
 //   AG_Window *skeletonVpToolsWin;
   // AG_Window *dataSizeWinxy;
//    AG_Window *dataSizeWinxz;
 //	AG_Window *navWin;
//	AG_Window *toolsWin;
    //AG_Window *aboutWin;

   // AG_Window *openFileDlgWin;
   // AG_FileDlg *fileDlgOpenSkel;

    //AG_Window *saveAsFileDlgWin;
   // AG_FileDlg *fileDlgSaveAsSkel;

   // AG_Window *loadImgJTableWin;
   // AG_FileDlg *fileDlgOpenImgJTable;


    //Console *console;
   //CommentsWidget *commentsWidget;
    //TracingTimeWidget *tracingTimeWidget;

    //NavigationWidget *navigationWidget;
    //ToolsWidget *toolWidget;

    /*Viewport *vpXyWin;
    Viewport *vpXzWin;
    Viewport *vpYzWin;
    Viewport *vpSkelWin; */

    //AG_FileDlg *fileDlg;

    //AG_Numerical *actNodeIDWdgt1;
   // AG_Numerical *actNodeIDWdgt2;

    //Labels in Viewports for DataSize
    //AG_Label *dataSizeLabelxy;
    //AG_Label *dataSizeLabelxz;
    //AG_Label *dataSizeLabelyz;

    //Labels in tracingTimeWin
    //AG_Label *runningTime;
    //AG_Label *tracingTime;
    //AG_Label *idleTime;

    //Chars for commentsWin

    char *comment1;
    char *comment2;
    char *comment3;
    char *comment4;
    char *comment5;

    //Zoom for Skeleton Viewport
    float zoomSkeletonViewport;
    float zoomOrthoVPs;

    //AG_Checkbox *vpLabelBox;
    //AG_Checkbox *highlightActiveTreeBox;
    //AG_Checkbox *showAllNodeIDsBox;
    //AG_Checkbox *datasetLinearFilteringBox;
    //AG_Checkbox *AutoTracingBox;

    // substrings for comment node preferences
    char *commentSubstr[NUM_COMMSUBSTR];
    // colors of color-dropdown in comment node preferences
    char* commentColors[NUM_COMMSUBSTR];

};

/**
  * @struct vpConfig
  * @brief Contains attributes for widget size, screen pixels per data pixels,
  *        as well as flags about user interaction with the widget
  */
struct vpConfig {
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

    // type e {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ, VIEWPORT_SKELETON}
    Byte type;
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
    struct vpConfig *vpConfigs;
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

    //Flag to indicate user repositioning
	uint userRepositioning;

	float depthCutOff;

    //Flag to indicate active mouse motion tracking: 0 off, 1 on
    int motionTracking;

	//Stores the current window size in screen pixels
    uint screenSizeX;
    uint screenSizeY;

    uint activeVP;

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
    uint workMode;
    bool superCubeChanged;

    struct guiConfig *gui;
    struct inputmap *inputmap;

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





     // This array holds the table for overlay coloring.
     // The colors should be "maximally different".

    GLuint overlayColorMap[4][256];

    bool overlayVisible;


    // Advanced Tracing Modes Stuff

    bool autoTracingEnabled;
    int autoTracingMode;
    int autoTracingDelay;
    int autoTracingSteps;

    // flags for changing vp position or size. Can be
    // VIEWPORT_XY, ...,
    // -1 for default position and size,
    // 10 for knossos start
    int moveVP;
    int resizeVP;
    int saveCoords;
    Coordinate clickedCoordinate;

    float cumDistRenderThres;

    int showPosSizButtons;
    int viewportOrder[4];
};

struct commentListElement {
    struct commentListElement *next;
    struct commentListElement *previous;

    char *content;

    struct nodeListElement *node;
};

struct treeListElement {
    struct treeListElement *next;
    struct treeListElement *previous;

    struct nodeListElement *firstNode;

    int treeID;
    color4F color;
    int colorSetManually;

    char comment[8192];
};

struct nodeListElement {
    struct nodeListElement *next;
    struct nodeListElement *previous;

    struct segmentListElement *firstSegment;

    struct treeListElement *correspondingTree;

    float radius;

    //can be VIEWPORT_XY, ...
    Byte createdInVp;
    int createdInMag;
    int timestamp;

    struct commentListElement *comment;

    // counts forward AND backward segments!!!
    int numSegs;

   // circumsphere radius - max. of length of all segments and node radius.
   //Used for frustum culling
   float circRadius;

    uint nodeID;
    Coordinate position;
    int isBranchNode;
};


struct segmentListElement {
    struct segmentListElement *next;
    struct segmentListElement *previous;

    //Contains the reference to the segment inside the target node
    struct segmentListElement *reverseSegment;

    // 1 signals forward segment 2 signals backwards segment.
    // Use SEGMENT_FORWARD and SEGMENT_BACKWARD.
    int flag;

    float length;

    char *comment;

    //Those coordinates are not the same as the coordinates of the source / target nodes
    //when a segment crosses the skeleton DC boundaries. Then these coordinates
    //lie at the skeleton DC borders. This applies only, when we use the segment inside
    //a skeleton DC, not inside of a tree list.
    //Coordinate pos1;
    //Coordinate pos2;

    struct nodeListElement *source;
    struct nodeListElement *target;
};

struct skeletonDC {
    struct skeletonDCsegment *firstSkeletonDCsegment;
    struct skeletonDCnode *firstSkeletonDCnode;
};

struct skeletonDCnode {
    struct nodeListElement *node;
    struct skeletonDCnode *next;
};

struct skeletonDCsegment {
    struct segmentListElement *segment;
    struct skeletonDCsegment *next;
};

struct peerListElement {
    uint id;
    char *name;
    floatCoordinate scale;
    Coordinate offset;

    struct peerListElement *next;
};

struct IOBuffer {
    uint size;      // The current maximum size
    uint length;    // The current amount of data in the buffer
    Byte *data;
};

typedef struct {
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
} mesh;

typedef struct {
        GLbyte r;
        GLbyte g;
        GLbyte b;
        GLbyte a;
} color4B;

/**
  * @struct skeletonState
  */
struct skeletonState {
    uint skeletonRevision;

    //    skeletonTime is the time spent on the current skeleton in all previous
    //    instances of knossos that worked with the skeleton.
    //    skeletonTimeCorrection is the time that has to be subtracted from
    //    SDL_GetTicks() to yield the number of milliseconds the current skeleton
    //    was loaded in the current knossos instance.

    bool unsavedChanges;
    int skeletonTime;
    int skeletonTimeCorrection;

    int idleTimeTicksOffset;
    int idleTimeLoadOffset;
    int idleTimeSession;
    int idleTime;
    int idleTimeNow;
    int idleTimeLast;

    Hashtable *skeletonDCs;
    struct treeListElement *firstTree;
    struct treeListElement *activeTree;
    struct nodeListElement *activeNode;

    struct commentListElement *currentComment;
    char *commentBuffer;
    char *searchStrBuffer;

    struct stack *branchStack;

    struct dynArray *nodeCounter;
    struct dynArray *nodesByNodeID;

    uint skeletonDCnumber;
    uint workMode;
    uint volBoundary;

    uint numberComments;

    unsigned int userCommentColoringOn;
    unsigned int commentNodeRadiusOn;

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

    // Display list,
    //which renders the skeleton defined in skeletonDisplayMode
    //(may be same as in displayListSkeletonSkeletonizerVPSlicePlaneVPs
    GLuint displayListSkeletonSkeletonizerVP;
    // Display list, which renders the skeleton of the slice plane VPs
    GLuint displayListSkeletonSlicePlaneVP;
    // Display list, which applies the basic openGL operations before the skeleton is rendered.
    //(Light settings, view rotations, translations...)
    GLuint displayListView;
    // Display list, which renders the 3 orthogonal slice planes, the coordinate axes, and so on
    GLuint displayListDataset;

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];

    // Stores the angles of the cube in the SkeletonVP
    float rotationState[16];
    // The next three flags cause recompilation of the above specified display lists.

    //TRUE, if all display lists must be updated
    bool skeletonChanged;
    //TRUE, if the view on the skeleton changed
    bool viewChanged;
    //TRUE, if dataset parameters (size, ...) changed
    bool datasetChanged;
    //TRUE, if only displayListSkeletonSlicePlaneVP must be updated.
    bool skeletonSliceVPchanged;

    //uint skeletonDisplayMode;
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

    int greatestNodeID;
    int greatestTreeID;

    color4F commentColors[NUM_COMMSUBSTR];
    float commentNodeRadii[NUM_COMMSUBSTR];

    //If TRUE, loadSkeleton merges the current skeleton with the provided
    int mergeOnLoadFlag;

    uint lastSaveTicks;
    bool autoSaveBool;
    uint autoSaveInterval;
    uint saveCnt;
    char *skeletonFile;
    char * prevSkeletonFile;

    char *deleteSegment;

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

    struct cmdList *undoList;
    struct cmdList *redoList;

    QString skeletonFileAsQString;
};

struct clientState {
    bool connectAsap;
    int remotePort;
    bool connected;
    Byte synchronizePosition;
    Byte synchronizeSkeleton;
    int connectionTimeout;
    bool connectionTried;
    char serverAddress[1024];

    QHostAddress *remoteServer;
    QTcpSocket *remoteSocket;
    QSet<QTcpSocket *> *socketSet;
    uint myId;
    bool saveMaster;

    struct peerListElement *firstPeer;
    struct IOBuffer *inBuffer;
    struct IOBuffer *outBuffer;
};

/**
  * @struct inputmap
  * Maybe obsolet in QT
  */
struct inputmap {
    int mouse;
    int modkey;
    int key;

    int action;

    struct inputmap *next;
};

/*
 *
 * Commands for undo and redo
 *
 */

struct cmdList {
    int cmdCount;
    struct cmdListElement *firstCmd;
    struct cmdListElement *lastCmd;
};

struct cmdListElement {
    void *cmd;
    int cmdType; //see command type constants
    struct cmdListElement *prev;
    struct cmdListElement *next;
};

typedef struct {
    struct nodeListElement *deletedNode;
} cmdDelNode;

typedef struct {
    int prevActiveNodeID;
    struct nodeListElement *newNode;
    int oldWorkMode;
    int newWorkMode;
} cmdAddNode;

typedef struct {
    int NodeID1;
    int NodeID2;
} cmdLinkNode;

typedef struct {
    int NodeID1;
    int NodeID2;
} cmdUnlinkNode;

typedef struct {
    int popNodeID;
    int oldActiveNodeID;
} cmdPopBranch;

typedef struct {
    int NodeID;
} cmdPushBranchNode;

typedef struct {
    int oldActiveNodeID;
    int newActiveNodeID;
} cmdChangeActiveNode;

typedef struct {
    int oldActiveTreeID;
    int newActiveTreeID;
} cmdChangeActiveTree;

typedef struct {
    int treeID;
} cmdAddTree;


typedef struct {

} cmdDelTree;

typedef struct {
    struct skeletonState *skelState;
} cmdSplitTree;

typedef struct {

} cmdMergeTree;

typedef struct {
    int treeID;
    color4F oldColor;
} cmdChangeTreeColor;

typedef struct {
    int nodeID;
    char* oldComment;
    char* newComment;
} cmdChangeComment;

typedef struct {
    int nodeID;
    char* comment;
} cmdDelComment;

typedef struct {
    int nodeID;
    char* comment;
} cmdAddComment;

/* global functions */

/* Macros */
#define SET_COLOR(color, rc, gc, bc, ac) \
        { \
        color.r = rc; \
        color.g = gc; \
        color.b = bc; \
        color.a = ac; \
        }

#define SET_COORDINATE(coordinate, a, b, c) \
        { \
        coordinate.x = a; \
        coordinate.y = b; \
        coordinate.z = c; \
        }

#define COMPARE_COORDINATE(c1, c2)  (((c1).x == (c2).x) && ((c1).y == (c2).y) && ((c1).z == (c2).z))

#define ADD_COORDINATE(c1, c2) \
	{ \
			(c1).x += (c2).x; \
			(c1).y += (c2).y; \
			(c1).z += (c2).z; \
	}

#define SUB_COORDINATE(c1, c2) \
	{ \
			(c1).x -= (c2).x; \
			(c1).y -= (c2).y; \
			(c1).z -= (c2).z; \
	}

#define DIV_COORDINATE(c1, c2) \
	{ \
			(c1).x /= (c2); \
			(c1).y /= (c2); \
			(c1).z /= (c2); \
	}

#define CPY_COORDINATE(c1, c2) \
	{ \
			(c1).x = (c2).x; \
			(c1).y = (c2).y; \
			(c1).z = (c2).z; \
	}

// This is used by the hash function. It rotates the bits by n to the left. It
// works analogously to the 8086 assembly instruction ROL and should actually
// compile to that instruction on a decent compiler (confirmed for gcc).
// ONLY FOR UNSIGNED 32-BIT INT, of course.

#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// Compute a % b with b being a power of two. This works because a % b will
// preserve all the bits less significant than the most significant bit
// of b (if b is a power of two).

#define MODULO_POW2(a, b)   (a) & ((b) - 1)
#define COMP_STATE_VAL(val) (state->val == tempConfig->val)
#define LOG(...) \
    { \
    printf("[%s:%d] ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
    }
    /*if(state->viewerState->viewerReady) \
       AG_ConsoleMsg(state->viewerState->gui->agConsole, __VA_ARGS__); \*/

#endif
