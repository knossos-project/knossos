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

/*
 *	Very general stuff.
 */

#include <SDL/SDL_net.h>
#include <GL/gl.h>

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <agar/core.h>
#include <agar/gui.h>


#define KVERSION "3.4.1"

#define TRUE    1
#define FALSE   0
#define FAIL    -1

#define MIN_N   1

#define PI 3.141592653589793238462643383279

#define TEXTURE_EDGE_LEN 1024

#define NUM_MAG_DATASETS 65536

#ifdef ARCH_64
#define PTRSIZEINT int64_t
#else
#define PTRSIZEINT int32_t
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

/*
 *	For the hashtable.
 */

#define HT_SUCCESS  1
#define HT_FAILURE  0
#define LL_SUCCESS  1
#define LL_FAILURE  0
#define LL_BEFORE   0
#define LL_AFTER    2

/*
 *	For the viewer.
 */

#define	SLICE_XY	0
#define SLICE_XZ	1
#define	SLICE_YZ	2

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

/* MAG is a bit unintiutive here: a lower MAG means in KNOSSOS that a
* a pixel of the lower MAG dataset has a higher resolution, i.e. 10 nm
* pixel size instead of 20 nm */
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

/*
 * For the Lookup tables
 */
 #define RGB_LUTSIZE  768
 #define RGBA_LUTSIZE 1024
 #define MAX_COLORVAL  255.

/*
 *  For the GUI.
 */
#define MAX_RECENT_FILES 10
/*
 *	For the remote.
 */

#define MAX_BUFFER_SIZE 52428800 // 50 MiB

#define USEREVENT_JUMP 5
#define USEREVENT_MOVE 6
#define USEREVENT_REDRAW 7
#define USEREVENT_NOAUTOSAVE 8
#define USEREVENT_REALQUIT 9

#define REMOTE_TRAJECTORY 5
#define REMOTE_FOLLOW 6
#define REMOTE_SYNCHRONIZE 7
#define REMOTE_RECENTERING 8

/*
 * For the client / server protocol
 */

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


/*
 * For custom key bindings
 */

#define ACTION_NONE 0

/*
 * For the skeletonizer
 */

#define SKELETONIZER_ON_CLICK_ADD_NODE 0
#define SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE 1
#define SKELETONIZER_ON_CLICK_DEL_NODE 2
#define SKELETONIZER_ON_CLICK_DROP_NODE 3
#define SEGMENT_FORWARD 1
#define SEGMENT_BACKWARD 2

/*
#define DISPLAY_WHOLE_SKELETON 0
#define DISPLAY_CURRENTCUBE_SKELETON 1
#define DISPLAY_ACTIVETREE_SKELETON 2
#define DISPLAY_NOTHING_SKELETON 3
#define DISPLAY_ONLYSLICEPLANE_SKELETON 4
#define DISPLAY_SEGS_AS_LINES 5
#define DISPLAY_LINES_POINTS_ONLY 6
*/

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

/*
 * For the renderer
 */

#define CURRENT_MAG_COORDINATES     0
#define ORIGINAL_MAG_COORDINATES    1


#define AUTOTRACING_NORMAL      0 //no additional move, recenter on new node
#define AUTOTRACING_VIEWPORT    1 //additional move of autoTracingSteps along clicked viewport
#define AUTOTRACING_TRACING     2 //additional move of autoTracingSteps along tracing direction
#define AUTOTRACING_MIRROR      3 //additional move of steps equal to distance between last and new node along tracing direction

#define ROTATIONSTATERESET 0
#define ROTATIONSTATEXY 1
#define ROTATIONSTATEXZ 3
#define ROTATIONSTATEYZ 2


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

/*
 *
 *      Structures and custom types
 *
 */

typedef uint8_t Byte;

typedef struct {
        int32_t x;
        int32_t y;
        int32_t z;
} Coordinate;

typedef struct {
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat a;
} color4F;

typedef struct {
		float x;
		float y;
		float z;
} floatCoordinate;

struct _CubeSlot {
        Byte *cube;
        struct _CubeSlot *next;
        struct _CubeSlot *previous;
};

typedef struct _CubeSlot CubeSlot;

struct _CubeSlotList {
        CubeSlot *firstSlot;
        int32_t elements;
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
//   next element if that element has the same hash as the current element and is
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

typedef struct {
        C2D_Element *listEntry;
        uint32_t tablesize;
        C2D_Element **table;
} Hashtable;

// This is used for a linked list of datacube slices that have to be processed for a given viewport.
// A backlog is generated when we want to retrieve a specific slice from a dc but that dc
// is unavailable at that time.
struct vpBacklogElement {
	Byte *slice;
    Coordinate cube;
	// I guess those aren't really necessary.
	uint32_t x_px;
	uint32_t y_px;
    uint32_t dcOffset;
    uint32_t cubeType;
    struct vpBacklogElement *next;
    struct vpBacklogElement *previous;
};

struct vpBacklog {
    struct vpBacklogElement *entry;
    uint32_t elements;
};

// This is used for a (counting) linked list of viewports that have to be handled
// (their textures put together from datacube slices)
struct vpListElement {
    struct viewPort *viewPort;
    struct vpBacklog *backlog;
    struct vpListElement *next;
    struct vpListElement *previous;
};

struct vpList {
    struct vpListElement *entry;
    uint32_t elements;
};

struct stack {
    uint32_t elementsOnStack;
    void **elements;
    int32_t stackpointer;
    int32_t size;
};

struct dynArray {
    void **elements;
    int32_t end;
    int32_t firstSize;
};

struct assignment {
    char *lval;
    char *rval;
};


// This structure holds everything we need to know about the current instance of
// KNOSSOS.

struct stateInfo {
/*
 * Info about the data
 */

        /* stores the currently active magnification;
        * it is set by magnification = 2^MAGx
        * state->magnification should only be used by the viewer,
        * but its value is copied over to loaderMagnification.
        * This is locked for thread safety. */
        int32_t magnification;

        uint32_t highestAvailableMag;
        uint32_t lowestAvailableMag;

        /* This variable is used only by the loader.
        * It is filled by the viewer and contains
        * log2uint32(state->magnification)  */
        uint32_t loaderMagnification;

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

        // Bytes in one datacube: 2^3N
        uint32_t cubeBytes;

        // Edge length of one cube in pixels: 2^N
        int32_t cubeEdgeLength;

		// Area of a cube slice in pixels;
		int32_t cubeSliceArea;

        // Supercube edge length in datacubes.
		int32_t M;
		uint32_t cubeSetElements;

        // Cube hierarchy mode
        int32_t boergens;

        // Bytes in one supercube (This is pretty much the memory
        // footprint of KNOSSOS): M^3 * 2^3M
        uint32_t cubeSetBytes;

        // Use overlay cubes to color the data.
        // Values: TRUE, FALSE.
        uint32_t overlay;

		/*
		 * Inter-thread communication structures / signals / mutexes, etc.
		 */

		 /*
		 * Tells the loading thread, that state->path and or state->name changed
		 */
		 int32_t datasetChangeSignal;


        // Tell the loading thread that it should interrupt its work /
        // its sleep and do something new.
        int32_t loadSignal;

        // If loadSignal is TRUE and quitSignal is TRUE, make the
        // loading thread quit. loadSignal == TRUE means the loader
        // has been signalled. If quitSignal != TRUE, it will go on
        // loading its stuff.
        int32_t quitSignal;

		// These signals are used to communicate with the remote.
		int32_t remoteSignal;

        // Same for the client. The client threading code is basically
        // copy-pasted from the remote.
        int32_t clientSignal;

		int32_t maxTrajectories;

        // Tell the loading thread to wake up.
        SDL_cond *conditionLoadSignal;

		// Tell the remote to wake up.
		SDL_cond *conditionRemoteSignal;

        SDL_cond *conditionClientSignal;

        // Any signalling to the loading thread needs to be protected
        // by this mutex. This is done by sendLoadSignal(), so always
        // use sendLoadSignal() to signal to the loading thread.
        SDL_mutex *protectLoadSignal;

		// This should be accessed through sendRemoteSignal() only.
        SDL_mutex *protectRemoteSignal;

        // Access through sendClientSignal()
        SDL_mutex *protectClientSignal;

        // ANY access to the Dc2Pointer or Oc2Pointer tables has
        // to be locked by this mutex.
        SDL_mutex *protectCube2Pointer;

        /*
         * Protect the network output buffer and the network peers list
         */
        SDL_mutex *protectOutBuffer;
        SDL_mutex *protectPeerList;

        /*
         * Protect changes to the skeleton for network synchronization.
         */
        SDL_mutex *protectSkeleton;

		/*
		 * Info about the state of KNOSSOS in general.
		 */

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
		struct remoteState *remoteState;
        struct clientState *clientState;
		struct loaderState *loaderState;
		struct skeletonState *skeletonState;
		struct skeletonCaretaker *skelCaretaker;
		struct trajectory *trajectories;
};

struct trajectory {
		char name[64];
		char *source;
};

struct loaderState {
	Hashtable *Dcoi;

	CubeSlotList *freeDcSlots;
    CubeSlotList *freeOcSlots;
	Byte *DcSetChunk;
    Byte *OcSetChunk;
    Byte *bogusDc;
    Byte *bogusOc;
};

struct viewPortTexture {
    //Handles for OpenGl
    uint32_t texHandle;
    uint32_t overlayHandle;

    //The absPx coordinate of the upper left corner of the texture actually stored in *texture
    Coordinate leftUpperPxInAbsPx;
    uint32_t edgeLengthDc;
    uint32_t edgeLengthPx;

    //These variables specifiy the area inside the textures which are used
    //for data storage. Storage always starts at texture pixels (0,0).
    uint32_t usedTexLengthDc;
    uint32_t usedTexLengthPx;

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

struct agConfig {
    char settingsFile[2048];
    char titleString[2048];

    char recentFiles[MAX_RECENT_FILES][4096];
    AG_MenuItem *appMenuRoot;

    /* Current position of the user crosshair,
    starting at 1 instead 0. This is shown to the user,
    KNOSSOS works internally with 0 start indexing. */
	Coordinate oneShiftedCurrPos;
	Coordinate activeNodeCoord;

    int32_t yesNoReturn;

	/* tools win buffer variables */
	int32_t activeNodeID;
	int32_t activeTreeID;
	int32_t totalNodes;
    int32_t totalTrees;
    int32_t numBranchPoints;
    char *commentBuffer;
    char *commentSearchBuffer;
    char *treeCommentBuffer;


    int32_t mergeTreesID1;
    int32_t mergeTreesID2;

    int32_t linkActiveWithNode;
    int32_t useLastActNodeRadiusAsDefault;
    float actNodeRadius;

    color4F actTreeColor;

    /* File dialog widget variables and buffers */
    AG_FileType *fileTypeNml;

    char skeletonDirectory[2048];
    char datasetLUTDirectory[2048];
    char datasetImgJDirectory[2048];
    char treeImgJDirectory[2048];
    char treeLUTDirectory[2048];
    char customPrefsDirectory[2048];
    char treeLUTFile[2048];
    char datasetLUTFile[2048];

    /* file saving settings win buffer variables */
    uint32_t autoSaveInterval; /* in minutes */
    int onSaveAutoIncrement;

    /* viewport pref window */
    int enableLinearFiltering;
    int radioRenderingModel;
    int enableOrthoSkelOverlay;
    int radioSkeletonDisplayMode;

    /* synchronization settings win buffer variables */

    /* dataset navigation settings win buffer variables */
    uint32_t stepsPerSec;
    uint32_t recenteringTime;
    uint32_t dropFrames;

    /* skeleton statistics win buffer variables */

    int agInputWdgtFocused;

    AG_Window *agWin;

    AG_Window *navOptWin;
    AG_Window *dispOptWin;
    AG_Window *syncOptWin;
    AG_Label *syncOptLabel;
    AG_Window *saveOptWin;
    AG_Window *renderingOptWin;
    AG_Window *spatLockOptWin;
    AG_Window *volTraceOptWin;
    AG_Window *dataSetStatsWin;
    AG_Window *viewPortPrefWin;
    AG_Window *zoomingWin;
    AG_Window *tracingTimeWin;
    AG_Window *commentsWin;
    AG_NotebookTab *commentNodesTab;
    AG_Table *commentsTable;
    AG_Window *setDynRangeWin;
	AG_Window *coordBarWin;
    AG_Window *skeletonVpToolsWin;
    AG_Window *dataSizeWinxy;
    AG_Window *dataSizeWinxz;
    AG_Window *dataSizeWinyz;
	AG_Window *navWin;
	AG_Window *toolsWin;
	AG_Window *aboutWin;

    AG_Window *openFileDlgWin;
    AG_FileDlg *fileDlgOpenSkel;

    AG_Window *saveAsFileDlgWin;
    AG_FileDlg *fileDlgSaveAsSkel;

    AG_Window *loadImgJTableWin;
    AG_FileDlg *fileDlgOpenImgJTable;

    AG_Window *consoleWin;
    AG_Console *agConsole;

    AG_Window *vpXyWin;
    AG_GLView *glViewXy;

    AG_Window *vpXzWin;
    AG_GLView *glViewXz;

    AG_Window *vpYzWin;
    AG_GLView *glViewYz;

    AG_Window *vpSkelWin;
    AG_GLView *glViewSkel;

    AG_FileDlg *fileDlg;

    AG_Numerical *actNodeIDWdgt1;
    AG_Numerical *actNodeIDWdgt2;

    //Labels in Viewports for DataSize
    AG_Label *dataSizeLabelxy;
    AG_Label *dataSizeLabelxz;
    AG_Label *dataSizeLabelyz;

    AG_Window *VpPosAndSizWin[4];

    //Labels in tracingTimeWin
    AG_Label *runningTime;
    AG_Label *tracingTime;
    AG_Label *idleTime;

    //for comment shortcuts
    char *comment1;
    char *comment2;
    char *comment3;
    char *comment4;
    char *comment5;

    // substrings for comment node preferences
    char *commentSubstr[NUM_COMMSUBSTR];
    // colors of color-dropdown in comment node preferences
    char* commentColors[NUM_COMMSUBSTR];
    char *tableCommentBuffer;

    int32_t filterBranchNodesOnly;
    //Zoom for Skeleton Viewport
	float zoomSkeletonViewport;
    float zoomOrthoVPs;

	AG_Checkbox *vpLabelBox;
	AG_Checkbox *highlightActiveTreeBox;
    AG_Checkbox *showAllNodeIDsBox;
    AG_Checkbox *datasetLinearFilteringBox;
    AG_Checkbox *AutoTracingBox;

    AG_Checkbox *showPosSizButtonsCheckbox;
};

struct viewPort {
    struct viewPortTexture texture;

    //The absPx coordinate of the upper left corner pixel of the currently on screen displayed data
    Coordinate leftUpperDataPxOnScreen;

    //This is a bit confusing..the screen coordinate system has always
    //x on the horizontal and y on the verical axis, but the displayed
    //data pixels can have a different axis. Keep this in mind.
    //These values depend on texUnitsPerDataPx (in struct viewPortTexture),
    //the current zoom value and the data pixel voxel dimensions.
    float screenPxXPerDataPx;
    float screenPxYPerDataPx;

    // These are computed from screenPx?PerDataPx and are used to convert
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

    uint32_t edgeLength;

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

    GLuint displayList; // not used anymore?

    //Variables that store the mouse "move path length". This is necessary, because not every mouse move pixel
    //would result in a data pixel movement
    float userMouseSlideX;
    float userMouseSlideY;

};

struct viewerState {
    struct viewPort *viewPorts;
    Byte *texData;
    Byte *overlayData;
    Byte *defaultTexData;
    Byte *defaultOverlayData;
    uint32_t numberViewPorts;
    uint32_t splash;
    uint32_t viewerReady;
    GLuint splashTexture;
    //Flag to indicate user movement
	uint32_t userMove;
    int32_t highlightVp;
    int32_t vpKeyDirection[3];


    /* don't jump between mags on zooming */
    int datasetMagLock;

    //Flag to indicate user repositioning
	uint32_t userRepositioning;

	float depthCutOff;

    //Flag to indicate active mouse motion tracking: 0 off, 1 on
    int motionTracking;

	//Stores the current window size in screen pixels
    uint32_t screenSizeX;
    uint32_t screenSizeY;

    uint32_t activeVP;

	/* Current position of the user crosshair.
       Given in pixel coordinates of the current local dataset (whatever magnification
       is currently loaded.) */
	Coordinate currentPosition;

    uint32_t recenteringTime;
    uint32_t recenteringTimeOrth;
    uint32_t walkOrth;

	SDL_Surface *screen;

    //Keyboard repeat rate
    uint32_t stepsPerSec;
	GLint filterType;
    int multisamplingOnOff;
    int lightOnOff;

    // Draw the colored lines that highlight the orthogonal VP intersections with each other.
    int drawVPCrosshairs;

    //Show height/width-labels inside VPs
    int showVPLabels;

    int selectModeFlag;

    uint32_t dropFrames;
    uint32_t walkFrames;

    float voxelDimX;
    float voxelDimY;
    float voxelXYRatio;
    float voxelDimZ;
    //YZ can't be different to XZ because of the intrinsic properties of the SBF-SEM.
    float voxelXYtoZRatio;

    float cumDistRenderThres;

    // allowed are: ON_CLICK_RECENTER 1, ON_CLICK_DRAG 0
    uint32_t workMode;
    int superCubeChanged;

    struct agConfig *ag;
    struct inputmap *inputmap;

    int32_t luminanceBias;
    int32_t luminanceRangeDelta;

    GLuint datasetColortable[3][256];
    GLuint datasetAdjustmentTable[3][256];
    int datasetColortableOn;
    int datasetAdjustmentOn;
    GLuint neutralDatasetTable[3][256];

    int treeLutSet;
    int treeColortableOn;
    float treeColortable[RGB_LUTSIZE];
    float treeAdjustmentTable[RGB_LUTSIZE];
    float defaultTreeTable[RGB_LUTSIZE];


    /*
     * This array holds the table for overlay coloring.
     * The colors should be "maximally different".
     *
     */
    GLuint overlayColorMap[4][256];

    int overlayVisible;

    /*
    * Advanced Tracing Modes Stuff
    */
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

    int32_t treeID;
    color4F color;
    int32_t colorSetManually;

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
    int32_t createdInMag;
    int32_t timestamp;

    struct commentListElement *comment;

    /* counts forward AND backward segments!!! */
    int32_t numSegs;

    /* circumsphere radius - max. of length of all segments and node radius.
    Used for frustum culling */
    float circRadius;

    int32_t nodeID;
    Coordinate position;
    int32_t isBranchNode;
};


struct segmentListElement {
    struct segmentListElement *next;
    struct segmentListElement *previous;

    //Contains the reference to the segment inside the target node
    struct segmentListElement *reverseSegment;

    // 1 signals forward segment 2 signals backwards segment.
    // Use SEGMENT_FORWARD and SEGMENT_BACKWARD.
    int32_t flag;

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
    uint32_t id;
    char *name;
    floatCoordinate scale;
    Coordinate offset;

    struct peerListElement *next;
};

struct IOBuffer {
    uint32_t size;      // The current maximum size
    uint32_t length;    // The current amount of data in the buffer
    Byte *data;
};

typedef struct {
    floatCoordinate *vertices;
    floatCoordinate *normals;
    color4F *colors;

    /* useful for flexible mesh manipulations */
    uint32_t vertsBuffSize;
    uint32_t normsBuffSize;
    uint32_t colsBuffSize;
    /* indicates last used element in corresponding buffer */
    uint32_t vertsIndex;
    uint32_t normsIndex;
    uint32_t colsIndex;
} mesh;

typedef struct {
        GLbyte r;
        GLbyte g;
        GLbyte b;
        GLbyte a;
} color4B;

struct skeletonState {
    uint32_t skeletonRevision;

    /*
     *  skeletonTime is the time spent on the current skeleton in all previous
     *  instances of knossos that worked with the skeleton.
     *  skeletonTimeCorrection is the time that has to be subtracted from
     *  SDL_GetTicks() to yield the number of milliseconds the current skeleton
     *  was loaded in the current knossos instance.
     *
     */

    int32_t unsavedChanges;
    int32_t skeletonTime;
    int32_t skeletonTimeCorrection;

    int32_t idleTimeSession;
    int32_t idleTime;
    int32_t idleTimeNow;
    int32_t idleTimeLast;

    Hashtable *skeletonDCs;
    struct treeListElement *firstTree;
    struct treeListElement *activeTree;
    struct nodeListElement *activeNode;

    struct commentListElement *currentComment;
    char *commentBuffer;
    char *searchStrBuffer;
    char *filterCommentBuffer;

    struct stack *branchStack;

    struct dynArray *nodeCounter;
    struct dynArray *nodesByNodeID;

    uint32_t skeletonDCnumber;
    uint32_t workMode;
    uint32_t volBoundary;

    uint32_t numberComments;

    unsigned int userCommentColoringOn;
    unsigned int commentNodeRadiusOn;

	int lockPositions;
	uint32_t positionLocked;
	char onCommentLock[1024];
	Coordinate lockedPosition;
    long unsigned int lockRadius;

    float rotdx;
    float rotdy;
    int32_t rotationcounter;

    int32_t definedSkeletonVpView;

    float translateX, translateY;

    /* Display list, which renders the skeleton defined in skeletonDisplayMode
    (may be same as in displayListSkeletonSkeletonizerVPSlicePlaneVPs */
    GLuint displayListSkeletonSkeletonizerVP;
    /* Display list, which renders the skeleton of the slice plane VPs */
    GLuint displayListSkeletonSlicePlaneVP;
    /* Display list, which applies the basic openGL operations before the skeleton is rendered.
    (Light settings, view rotations, translations...) */
    GLuint displayListView;
    /* Display list, which renders the 3 orthogonal slice planes, the coordinate axes, and so on */
    GLuint displayListDataset;

    /* Stores the model view matrix for user performed VP rotations.*/
    float skeletonVpModelView[16];

    // Stores the angles of the cube in the SkeletonVP
    float rotationState[16];
    /* The next three flags cause recompilation of the above specified display lists. */

    //TRUE, if all display lists must be updated
    int skeletonChanged;
    //TRUE, if the view on the skeleton changed
    int viewChanged;
    //TRUE, if dataset parameters (size, ...) changed
    int datasetChanged;
    //TRUE, if only displayListSkeletonSlicePlaneVP must be updated.
    int skeletonSliceVPchanged;
    int commentsChanged;

    //uint32_t skeletonDisplayMode;
    uint32_t displayMode;

    float segRadiusToNodeRadius;
    int32_t overrideNodeRadiusBool;
    float overrideNodeRadiusVal;

    int highlightActiveTree;
    int showIntersections;
    int rotateAroundActiveNode;
    int showXYplane;
    int showXZplane;
    int showYZplane;
    int showNodeIDs;
    int32_t autoFilenameIncrementBool;

    int32_t treeElements;
    int32_t totalNodeElements;
    int32_t totalSegmentElements;

    int32_t greatestNodeID;
    int32_t greatestTreeID;

    color4F commentColors[NUM_COMMSUBSTR];
    float commentNodeRadii[NUM_COMMSUBSTR];
    struct nodeListElement *selectedCommentNode;

    //If TRUE, loadSkeleton merges the current skeleton with the provided
    int mergeOnLoadFlag;

    uint32_t lastSaveTicks;
    int32_t autoSaveBool;
    uint32_t autoSaveInterval;
    uint32_t saveCnt;
    char *skeletonFile;
    char * prevSkeletonFile;

    // char *deleteSegment; TDItem: is this needed?

    float defaultNodeRadius;

	// Current zoom level. 0: no zoom; near 1: maximum zoom.
	float zoomLevel;

    /* temporary vertex buffers that are available for rendering, get cleared
    every frame */
    mesh lineVertBuffer; /* ONLY for lines */
    mesh pointVertBuffer; /* ONLY for points */

    int branchpointUnresolved;

    /* This is for a workaround around agar bug #171*/
    int askingPopBranchConfirmation;
    char skeletonCreatedInVersion[32];
    char skeletonLastSavedInVersion[32];

    struct cmdList *undoList;
    struct cmdList *redoList;
};

struct remoteState {
		// type: REMOTE_TRAJECTORY, REMOTE_RECENTERING
		int32_t type;
		int32_t maxTrajectories;
		int32_t activeTrajectory;
        Coordinate recenteringPosition;
};

struct clientState {
    int32_t connectAsap;
    int32_t remotePort;
    int32_t connected;
    Byte synchronizePosition;
    Byte synchronizeSkeleton;
    int32_t connectionTimeout;
    int32_t connectionTried;
    char serverAddress[1024];
    IPaddress remoteServer;
    TCPsocket remoteSocket;
    SDLNet_SocketSet socketSet;

    uint32_t myId;
    uint32_t saveMaster;

    struct peerListElement *firstPeer;
    struct IOBuffer *inBuffer;
    struct IOBuffer *outBuffer;
};

struct inputmap {
    int32_t mouse;
    int32_t modkey;
    int32_t key;

    int32_t action;

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

struct skeletonMemento {
    // an array of messages to reconstruct a skeletonState
    struct dynArray *msgs;

	uint32_t skeletonRevision;
	uint32_t positionLocked;
	Coordinate lockedPosition;
};

struct skeletonCaretaker {
    struct dynArray *mementos;
};

/*
 *
 *      Macros
 *
 */
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
    }

/*       if(state->viewerState->viewerReady) \
        AG_ConsoleMsg(state->viewerState->ag->agConsole, __VA_ARGS__); \
 *
 *      Function prototypes
 *
 */

/*
 *	For coordinate.c
 */

Coordinate Px2DcCoord(Coordinate pxCoordinate);
int32_t transCoordinate(Coordinate *outCoordinate, int32_t x, int32_t y, int32_t z, floatCoordinate scale, Coordinate offset);
Coordinate *transNetCoordinate(uint32_t id, uint32_t x, uint32_t y, uint32_t z);

/*
 *	For hash.c
 */

// Create a new hashtable.
// tablesize specifies the size of the hash-to-data-table and should be 1.25
// to 1.5 times the number of elements the table is expected to store.
// If successful, the function returns a pointer to a Hashtable structure.
Hashtable *ht_new(uint32_t tablesize);

// Delete the whole hashtable and linked list and release all the memory.
// hashtable specifies which hashtable to delete.
// The return value is LL_SUCCESS or LL_FAILURE.
uint32_t ht_rmtable(Hashtable *hashtable);

// Return the value associated with a key.
// key is the key to look for.
// hashtable is the hashtable to look in.
// On success, a pointer to Byte (a Datacube) is returned, HT_FAILURE else.
Byte *ht_get(Hashtable *hashtable, Coordinate key);

// Insert an element.
uint32_t ht_put(Hashtable *hashtable, Coordinate key, Byte *value);

// Delete an element
uint32_t ht_del(Hashtable *hashtable, Coordinate key);
// Compute the union of h1 and h2 and put it in target.
uint32_t ht_union(Hashtable *target, Hashtable *h1, Hashtable *h2);

uint32_t nextpow2(uint32_t a);
uint32_t lastpow2(uint32_t a);

/*
 *	For knossos.c
 */
int32_t unlockSkeleton(int32_t increment);
int32_t lockSkeleton(int32_t targetRevision);
int32_t sendLoadSignal(uint32_t x, uint32_t y, uint32_t z, int32_t magChanged);
int32_t sendRemoteSignal();
int32_t sendClientSignal();
int32_t sendQuitSignal();
int32_t sendServerSignal();
uint32_t log2uint32(register uint32_t x);
uint32_t ones32(register uint32_t x);

/*
 *	For refreshTime.c
 */

int refreshTime();
void refreshTimeLabel();
void checkIdleTime();

/*
 *	For loader.c
 */

int loader();

/*
 * For remote.c
 */

int32_t remoteDelay(int32_t s);
int remote();
// For the sake of logical organization, we'd need another function
// remoteJumpTo. It would even somewhat simplify related functions.
int32_t remoteJump(int32_t x, int32_t y, int32_t z);
int32_t remoteWalk(int32_t x, int32_t y, int32_t z);
int32_t remoteWalkTo(int32_t x, int32_t y, int32_t z);
int32_t newTrajectory(char *trajName, char *trajectory);

/*
 *	For viewer.c
 */

int32_t loadDatasetColorTable(const char *path, GLuint *table, int32_t type);
int32_t loadTreeColorTable(const char *path, float *table, int32_t type);
int32_t updatePosition(int32_t serverMovement);
int32_t calcDisplayedEdgeLength();

/* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
uint32_t changeDatasetMag(uint32_t upOrDownFlag);


//Entry point for viewer thread, general viewer coordination, "main loop"
int viewer();

//Initializes the window with the parameter given in viewerState
uint32_t createScreen();

//Transfers all (orthogonal viewports) textures completly from ram (*viewerState->viewPorts[i].texture.data) to video memory
//Calling makes only sense after full initialization of the SDL / OGL screen
uint32_t initializeTextures();

//Frees allocated memory
uint32_t cleanUpViewer(struct viewerState *viewerState);

int32_t updateViewerState();
uint32_t userMove(int32_t x, int32_t y, int32_t z, int32_t serverMovement);
int32_t findVPnumByWindowCoordinate(uint32_t xScreen, uint32_t yScreen);
uint32_t moveVPonTop(uint32_t currentVP);
uint32_t recalcTextureOffsets();
int32_t refreshViewports();

/*
 * For eventHandler.c
 */
//Event handler function, which switches over the type of a given event and calls the appropriate downstream functions
uint32_t handleEvent(SDL_Event event);

/*
 * For renderer.c
 */
//This function is called when screen update is neccessary.
//It coordinates the drawing of the viewports in the appropriate order.
uint32_t drawGUI();
void updateAGconfig();
uint32_t renderOrthogonalVP(uint32_t currentVP);
uint32_t renderSkeletonVP(uint32_t currentVP);
uint32_t drawViewportProperties(uint32_t currentVP);
uint32_t retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width);

uint32_t doubleMeshCapacity(mesh *toDouble);
uint32_t initMesh(mesh *meshToInit, uint32_t initialSize);
//Some math helper functions
float radToDeg(float rad);
float degToRad(float deg);
float scalarProduct(floatCoordinate *v1, floatCoordinate *v2);
floatCoordinate *crossProduct(floatCoordinate *v1, floatCoordinate *v2);
float vectorAngle(floatCoordinate *v1, floatCoordinate *v2);
float euclidicNorm(floatCoordinate *v);
uint32_t normalizeVector(floatCoordinate *v);
int32_t roundFloat(float number);
int32_t sgn(float number);
uint32_t initRenderer();
uint32_t splashScreen();

/*
 * For skeletonizer.c
 */

uint32_t initSkeletonizer();
uint32_t UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
uint32_t UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive);
uint32_t updateSkeletonState();
int32_t updateTreeColors();
int32_t nextCommentlessNode();
int32_t previousCommentlessNode();


uint32_t updateCircRadius(struct nodeListElement *node);

uint32_t updateSkeletonFileName(int32_t targetRevision, int32_t increment, char *filename);
//uint32_t saveNMLSkeleton();
int32_t saveSkeleton();
//uint32_t loadNMLSkeleton();
uint32_t loadSkeleton();

void setDefaultSkelFileName();

uint32_t delActiveNode();
uint32_t delActiveTree();

uint32_t delSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID, struct segmentListElement *segToDel);
uint32_t delNode(int32_t targetRevision, int32_t nodeID, struct nodeListElement *nodeToDel);
uint32_t delTree(int32_t targetRevision, int32_t treeID);
struct nodeListElement *findNearbyNode(struct treeListElement *nearbyTree, Coordinate searchPosition);
struct nodeListElement *findNodeInRadius(Coordinate searchPosition);

uint32_t setActiveTreeByID(int32_t treeID);
uint32_t setActiveNode(int32_t targetRevision, struct nodeListElement *node, int32_t nodeID);
int32_t addNode(struct skeletonState *skeleton,
                int32_t targetRevision,
                int32_t nodeID,
                float radius,
                int32_t treeID,
                Coordinate *position,
                Byte VPType,
                int32_t inMag,
                int32_t time,
                int32_t respectLocks);
uint32_t addSegment(struct skeletonState *skeleton, int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID);

uint32_t clearSkeleton(int32_t targetRevision, int loadingSkeleton);

uint32_t mergeTrees(int32_t targetRevision, int32_t treeID1, int32_t treeID2);

void setNodeCoordinates(struct nodeListElement *node, int32_t x, int32_t y, int32_t z);
int32_t moveToPrevNode();
int32_t moveToNextNode();
struct nodeListElement *getNodeWithPrevID(struct nodeListElement *currentNode);
struct nodeListElement *getNodeWithNextID(struct nodeListElement *currentNode);
struct nodeListElement *findNodeByNodeID(int32_t nodeID);
struct nodeListElement *findNodeByCoordinate(Coordinate *position);
struct treeListElement *addTreeListElement(struct skeletonState *skeleton, int32_t sync, int32_t targetRevision, int32_t treeID, color4F color);
int32_t moveToPrevTree();
int32_t moveToNextTree();
struct treeListElement* getTreeWithPrevID(struct treeListElement *currentTree);
struct treeListElement* getTreeWithNextID(struct treeListElement *currentTree);
struct treeListElement *findTreeByTreeID(int32_t treeID);
int32_t addTreeComment(struct skeletonState *skeleton, int32_t targetRevision, int32_t treeID, char *comment);
struct segmentListElement *findSegmentByNodeIDs(int32_t sourceNodeID, int32_t targetNodeID);
int32_t editNode(int32_t targetRevision,
                 int32_t nodeID,
                 struct nodeListElement *node,
                 float newRadius,
                 int32_t newXPos,
                 int32_t newYPos,
                 int32_t newZPos,
                 int32_t inMag);
void *popStack(struct stack *stack);
int32_t pushStack(struct stack *stack, void *element);
struct stack *newStack(int32_t size);
int32_t delStack(struct stack *stack);
int32_t delDynArray(struct dynArray *array);
void *getDynArray(struct dynArray *array, int32_t pos);
int32_t setDynArray(struct dynArray *array, int32_t pos, void *value);
struct dynArray *newDynArray(int32_t size);
int32_t splitConnectedComponent(int32_t targetRevision, int32_t nodeID);
uint32_t addComment(struct skeletonState *skeleton, int32_t targetRevision, char *content, struct nodeListElement *node, int32_t nodeID);
uint32_t delComment(int32_t targetRevision, struct commentListElement *currentComment, int32_t commentNodeID);
uint32_t editComment(int32_t targetRevision, struct commentListElement *currentComment, int32_t nodeID, char *newContent, struct nodeListElement *newNode, int32_t newNodeID);
struct commentListElement *nextComment(char *searchString);
struct commentListElement *previousComment(char *searchString);
void setColorFromNode(struct nodeListElement *node, color4F *color);
void setRadiusFromNode(struct nodeListElement *node, float *radius);
unsigned int commentContainsSubstr(struct commentListElement *comment, int index);
uint32_t searchInComment(char *searchString, struct commentListElement *comment);
int32_t unlockPosition();
int32_t lockPosition(Coordinate lockCoordinate);
int32_t popBranchNode(int32_t targetRevision);
int32_t pushBranchNode(struct skeletonState *skeleton, int32_t targetRevision, int32_t setBranchNodeFlag, int32_t checkDoubleBranchpoint, struct nodeListElement *branchNode, int32_t branchNodeID);
uint32_t setSkeletonWorkMode(int32_t targetRevision, uint32_t workMode);
int32_t jumpToActiveNode();
void UI_popBranchNode();
void restoreDefaultTreeColor();
uint32_t genTestNodes(uint32_t number);
void resetSkeletonMeta();
//undo stuff
void undo();
int delSkelState(struct skeletonState *skelState);
int delTreesFromState(struct skeletonState *skelState);
int delTreeFromState(struct treeListElement *treeToDel, struct skeletonState *skelState);
int delNodeFromState(struct nodeListElement *nodeToDel, struct skeletonState *skelState);
int delCommentFromState(struct commentListElement *commentToDel, struct skeletonState *skelState);
int delSegmentFromCmd(struct segmentListElement *segToDel);

/*
 * For client.c
 */

int client();
uint32_t broadcastPosition(uint32_t x, uint32_t y, uint32_t z);
int32_t skeletonSyncBroken();
int32_t bytesToInt(Byte *source);
int32_t integerToBytes(Byte *dest, int32_t source);
int32_t floatToBytes(Byte *dest, float source);
int Wrapper_SDLNet_TCP_Open(void *params);
uint32_t IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint32_t length, SDL_mutex *mutex);
uint32_t addPeer(uint32_t id, char *name, float xScale, float yScale, float zScale, int32_t xOffset, int32_t yOffset, int32_t zOffset);
uint32_t delPeer(uint32_t id);
uint32_t broadcastCoordinate(uint32_t x, uint32_t y, uint32_t z);
int32_t syncMessage(char *fmt, ...);
int32_t clientSyncBroken();
int32_t parseInBufferByFmt(int32_t len, const char *fmt, float *f, Byte *s, int32_t *d, struct IOBuffer *buffer);

Byte *serialize(char *fmt, va_list args);

/*
 * For gui.c
 */

void quitKnossos();
int32_t initGUI();
uint32_t cpBaseDirectory(char *target, char *path, size_t len);
void yesNoPrompt(AG_Widget *par, char *promptString, void (*yesCb)(), void (*noCb)());
uint32_t addRecentFile(char *path, uint32_t pos);
void UI_workModeAdd();
void UI_workModeLink();
void UI_workModeDrop();
void saveSkelCallback(AG_Event *event);
void UI_saveSkeleton(int32_t increment);
void UI_saveSettings();
void UI_loadSkeleton(AG_Event *event);
void UI_pasteClipboardCoordinates();
void UI_copyClipboardCoordinates();
void UI_zoomOrthogonals(float step);
void UI_moveVP(int foundVP);
void UI_resizeVP(int foundVP);
void createToolsWin();
void createViewPortPrefWin();
void reloadDataSizeWin();
void updateTitlebar(int32_t useFilename);

 void createMenuBar();
 void createCoordBarWin();
 void createSkeletonVpToolsWin();
 void createDataSizeWin();
 void createNavWin();
 void createConsoleWin();
 void createAboutWin();

 void createNavOptionsWin();
 void createDisplayOptionsWin();
 void createSaveOptionsWin();
 void createSyncOptionsWin();
 void createRenderingOptionsWin();
 void createVolTracingOptionsWin();
 void createSpatialLockingOptionsWin();

 void createDataSetStatsWin();
 void createZoomingWin();
 void createTracingTimeWin();
 void createCommentsWin();
 void createLoadDatasetImgJTableWin();
 void createLoadTreeImgJTableWin();
 void createSetDynRangeWin();
 void treeColorAdjustmentsChanged();

 void setVPPosSizWinPositions();
 void UI_updateCommentsWin();
