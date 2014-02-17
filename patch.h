#ifndef PATCH_H
#define PATCH_H

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_3.h>

#include <pcl/point_types.h>
#include <pcl/impl/point_types.hpp>
#include <pcl/kdtree/kdtree.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/integral_image_normal.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/surface/gp3.h>
#include <pcl/point_cloud.h>
#include <pcl/io/ply_io.h>
#include <pcl/ros/conversions.h>
#include <pcl/Vertices.h>
#include <pcl/surface/marching_cubes.h>
#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/surface/marching_cubes_rbf.h>
#include <pcl/search/search.h>
#include <pcl/filters/passthrough.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/ModelCoefficients.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/passthrough.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/surface/poisson.h>
#include <pcl/surface/ear_clipping.h>
#include <pcl/surface/processing.h>
#include <pcl/surface/reconstruction.h>
#include <pcl/surface/convex_hull.h>
#include <pcl/surface/concave_hull.h>

#include <QObject>

#include "knossos-global.h"
#include "octree.h"
#include "skeletonizer.h"

typedef pcl::PointXYZ                       pcl_Point;
typedef pcl::PointCloud<pcl_Point>          pcl_Cloud;
typedef pcl::ConcaveHull<pcl_Point>         pcl_ConcaveHull;
typedef pcl::SACSegmentation<pcl_Point>     pcl_Segmentation;
typedef pcl::PolygonMesh                    pcl_Mesh;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_3<K>  Triangulation;
typedef K::Point_3                                  CGAL_Point;
typedef Triangulation::Facet                        CGAL_Facet;
typedef K::Triangle_3                               CGAL_Triangle;
typedef Triangulation::Cell_handle                  CGAL_CellHandle;
typedef Triangulation::Vertex_handle                CGAL_VertexHandle;

#define DRAW_CONTINUOUS_LINE 0 //! draw patch with a continuous line
#define DRAW_DROP_POINTS 1 //! draw patch by placing points
#define AUTO_ALIGN_RADIUS 2 //! radius in which two lines are connected at their end points

class PatchLoop : public QObject {
    Q_OBJECT
public:
    std::vector<floatCoordinate> points;
    std::vector<std::pair<Coordinate, Coordinate> > volumeStripes;
    floatCoordinate centroid;
    uint createdInVP;

    PatchLoop(uint inVP, QObject *parent = 0)
        : QObject(parent), createdInVP(inVP) {}

    PatchLoop(std::vector<floatCoordinate> newPoints, floatCoordinate newCentroid, uint inVP, QObject *parent = 0)
        : QObject(parent), centroid(newCentroid), createdInVP(inVP) {
        points = newPoints;
    }

    ~PatchLoop() {}

    void updateCentroid() {
        if(points.size() > 0) {
            centroid = centroidPolygon(points);
        }
    }
};

/**
 * @brief The Patch class: A patch is part of the volume annotation of a neuron. A tree consists of many patches,
 *        and a Patch consists of many volume loops and a surface.
 *        Patches are organized in a circular list.
 *
 */
class Patch : public QObject {
    Q_OBJECT

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW // this is necessary because Patch class holds an Eigen-member (meshCloud),
                                    // see http://eigen.tuxfamily.org/dox-devel/group__TopicUnalignedArrayAssert.html
                                    // and http://eigen.tuxfamily.org/dox-devel/group__TopicStructHavingEigenMembers.html
    // --- class members ---
    static bool patchMode; //! enables/disables volume annotation feature
    static uint drawMode; //! 'DRAW_CONTINUOUS_LINE' or 'DRAW_DROP_POINTS'
    static int eraseInVP; //! flag to indicate in which vp user wants to erase active loop. -1 if not
    static int eraserPosX; //! stores the current mouse position in viewport coordinates for eraser rendering
    static int eraserPosY; //! stores the current mouse position in viewport coordinates for eraser rendering
    static Patch *firstPatch; //! first patch of the circular doubly linked list.
                              //! If numPatches == 1, then firstPatch = firstPatch->next = firstPatch->previous
    static Patch *activePatch; //! the currently drawn patch
    static PatchLoop *activeLoop; //! holds the active patch's loop that is being edited.
    static uint maxPatchID; //! really the maxPatchID that ever existed. Needed to find next unused patchID
    static uint numPatches; //! number of patches

    static float voxelPerPoint; //! closeness of points to each other
    static float eraserHalfEdge; //! half edge length of the eraser in screen pixels
    static bool drawing; //! true if patchMode and right mouse button pressed in ortho vp
    static bool newPoints; //! true if a user draw has added new points to the cloud. Only re-triangulate if there are new points
    static GLuint vbo;
    static GLuint texHandle;
    static std::vector<floatCoordinate> activeLine; //! the currently drawn line.
                                                    //! Started on mouse down and added to 'lineBuffer' on mouse release
    static std::vector<std::vector<floatCoordinate> > lineBuffer; //! all lines of the not closed loop yet

    static uint displayMode; //! PATCH_DSP_WHOLE, PATCH_DSP_ACTIVE, PATCH_DSP_HIDE

    static Patch *newPatch(int patchID = -1);
    static void delPatch(Patch *patch);
    static Patch *getPatchWithID(uint patchID);
    static bool setActivePatch(uint patchID);
    static bool activateLoop(floatCoordinate center, float halfEdge, uint viewportType);
    static void delActivePatch();
    static void delActiveLoop();
    static bool insert(floatCoordinate point);
    static void addInterpolatedPoint(floatCoordinate p, floatCoordinate q, std::vector<floatCoordinate> &line);
    static void eraseActiveLoop(floatCoordinate center, uint viewportType);
    static bool activeLoopIsClosed();
    static void genRandCloud(uint cloudSize);
    static void genRandTriangulation(uint cloudSize);
    static void visiblePoints(uint viewportType);
    static void computeNormals();
    static void alignToLine(floatCoordinate point, int index, bool startPoint);
    static void lineFinished(floatCoordinate lastPoint, int viewportType);

    // ---------------------
    Patch *next;
    Patch *previous;
    uint patchID;
    treeListElement *correspondingTree;
    QString comment;
    floatCoordinate pos;
    bool visible;
    uint numPoints;
    uint numLoops;
    uint numTriangles;
    pcl_ConcaveHull triangulation;
    pcl_Mesh mesh;
    pcl_Cloud meshCloud;
    Octree<floatCoordinate> *pointCloud;
    Octree<Triangle> *triangles;
    Octree<PatchLoop*> *loops;
    Byte texData[TEXTURE_EDGE_LEN][TEXTURE_EDGE_LEN][4];
    Octree<Triangle> *distinguishableTrianglesSkelVP;
    Octree<Triangle> *distinguishableTrianglesOrthoVP;


    Patch(QObject *parent = 0, int newPatchID = -1);
    ~Patch();

    bool setTree(treeListElement *tree);
    Patch *nextPatchByID();
    Patch *previousPatchByID();
    bool allowPoint(floatCoordinate point);
    bool insert(Triangle triangle, bool replace);
    bool insert(PatchLoop *loop, uint viewportType);
    void computeVolume(int currentVP, PatchLoop *loop);
    std::vector<floatCoordinate> pointsOnLine(PatchLoop *loop, int x, int y, int z);
    std::vector<floatCoordinate> pointCloudAsVector(int viewportType = -1);
    std::vector<PatchLoop *> loopsAsVector(int viewportType = -1);
    std::vector<Triangle> trianglesAsVector(int viewportType = -1);
    std::vector<Triangle> distinguishableTrianglesAsVector(uint viewportType);
    bool computeTriangles();
    static pcl_Mesh::Ptr concaveHull(pcl_Cloud::Ptr cloudPtr);
    pcl_Mesh::Ptr marchingCubesSurface(pcl_Cloud::Ptr cloudPtr);
    pcl_Mesh::Ptr poissonReconstruction(pcl_Cloud::Ptr cloudPtr);
    void recomputeTriangles(floatCoordinate pos, uint halfCubeSize);
    void clearTriangles();

signals:
    void activePatchChanged();
public slots:
    void updateDistinguishableTriangles(int viewportType = -1);
    static void deactivateLoop();
};

#endif // PATCH_H
