#include <QElapsedTimer>

#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/surface/poisson.h>
#include <pcl/octree/octree.h>
#include <pcl/octree/octree_search.h>

#include "patch.h"
#include "functions.h"
#include "renderer.h"

extern struct stateInfo *state;

bool Patch::patchMode = false;
uint Patch::drawMode = DRAW_CONTINUOUS_LINE;
uint Patch::maxPatchID = 0;
uint Patch::numPatches = 0;
std::vector<floatCoordinate> Patch::activeLoop;
uint Patch::displayMode = PATCH_DSP_WHOLE;
Patch *Patch::firstPatch = NULL;
Patch *Patch::activePatch = NULL;
bool Patch::drawing = false;
bool Patch::newPoints = false;
GLuint Patch::vbo;
GLuint Patch::texHandle;


Patch::Patch(QObject *parent, int newPatchID) : QObject(parent),
    next(this), previous(this), numPoints(0), numTriangles(0) {

    if(newPatchID == -1) {
        patchID = ++Patch::maxPatchID;
    }
    else {
        patchID = newPatchID;
        if(Patch::maxPatchID < newPatchID) {
            maxPatchID = newPatchID;
        }
    }

    floatCoordinate center;
    center.x = state->boundary.x/2;
    center.y = state->boundary.y/2;
    center.z = state->boundary.z/2;
    float halfEdgeLength;
    if(center.x > center.y) {
        halfEdgeLength = center.x;
    }
    else {
        halfEdgeLength = center.y;
    }
    if(center.z > halfEdgeLength) {
        halfEdgeLength = center.z;
    }
    pointCloud = new Octree<floatCoordinate>(center, halfEdgeLength);
    triangles = new Octree<Triangle>(center, halfEdgeLength);
    distinguishableTrianglesOrthoVP = new Octree<Triangle>(center, halfEdgeLength);
    distinguishableTrianglesSkelVP = new Octree<Triangle>(center, halfEdgeLength);
    setTree(state->skeletonState->activeTree);

    texData = (Byte *)malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 4);
    memset(texData, 255, sizeof(texData)); // default is a transparent texture
}


Patch::~Patch() {
    delete pointCloud;
    delete triangles;
    delete distinguishableTrianglesOrthoVP;
    delete distinguishableTrianglesSkelVP;
}

bool Patch::setTree(treeListElement *tree) {
    if(tree != NULL) {
        correspondingTree = tree;
        return true;
    }
    return false;
}

Patch *Patch::nextPatchByID() {
    uint minDistance = UINT_MAX;
    uint tempMinDistance = minDistance;
    uint minID = UINT_MAX;
    Patch *patch = firstPatch;
    Patch *lowestPatch = NULL, *nextPatch = NULL;

    do {
        if(patch->patchID == this->patchID) {
            continue;
        }
        if(patch->patchID < minID) {
            lowestPatch = patch;
            minID = lowestPatch->patchID;
        }
        if(patch->patchID > this->patchID) {
            tempMinDistance = patch->patchID - this->patchID;
            if(tempMinDistance == 1) {
                return patch;
            }
            if(tempMinDistance < minDistance) {
                minDistance = tempMinDistance;
                nextPatch = patch;
            }
        }
        patch = patch->next;
    } while (patch != firstPatch);

    if(nextPatch == NULL) {
        nextPatch = lowestPatch;
    }
    return nextPatch;
}

Patch *Patch::previousPatchByID() {
    uint minDistance = UINT_MAX;
    uint tempMinDistance = minDistance;
    uint minID = UINT_MAX;
    Patch *patch = firstPatch;
    Patch *highestPatch = NULL, *prevPatch = NULL;

    do {
        if(patch->patchID == this->patchID) {
            qDebug("same");
            continue;
        }
        if(patch->patchID > minID) {
            highestPatch = patch;
            minID = highestPatch->patchID;
        }
        if(patch->patchID < this->patchID) {
            tempMinDistance = this->patchID - patch->patchID;
            if(tempMinDistance == 1) {
                return patch;
            }
            if(tempMinDistance < minDistance) {
                minDistance = tempMinDistance;
                prevPatch = patch;
            }
        }
        patch = patch->previous;
    } while (patch != firstPatch);

    if(prevPatch == NULL) {
        prevPatch = highestPatch;
    }
    return prevPatch;
}

/**
 * @brief Patch::allowPoint check if new point should be added to pointCloud
 * @param point the point for insertion
 * @return true if there is no other point in range, false otherwise.
 */
bool Patch::allowPoint(floatCoordinate point) {
    return !pointCloud->objInRange(point, 0);
}

/**
 * @brief Patch::insert insert a new triangle to this patch's surface
 * @param triangle the new triangle
 * @param replace define behaviour if a triangle at same location already exists: replace, or do nothing
 * @return true on insertion, false otherwise
 */
bool Patch::insert(Triangle *triangle, bool replace) {
    if(triangles->insert(triangle, centroidTriangle(*triangle), replace)) {
        numTriangles++;
        return true;
    }
    return false;
}

/**
 * @brief Patch::insert insert a new point to this patch's point cloud
 * @param point the new point for insertion
 * @param define behaviour if a point at same location already exists: replace, or do nothing
 * @return true on insertion, false otherwise
 */
bool Patch::insert(floatCoordinate *point, bool replace) {
    if(allowPoint(*point) == false) {
        return false;
    }
    if(pointCloud->insert(point, *point, replace)) {
        numPoints++;
        return true;
    }
    return false;
}

void Patch::delVisibleLoop(uint viewportType) {

}

/**
 * @brief Patch::pointCloudAsVector returns the point cloud as vector for easy handling
 */
std::vector<floatCoordinate> Patch::pointCloudAsVector(int viewportType) {
    std::vector<floatCoordinate> points;

    if(viewportType == -1) {
        pointCloud->getAllObjs(points);
    }
    else {
        pointCloud->getAllVisibleObjs(points, viewportType);
    }
    return points;
    /*std::vector<floatCoordinate> result;
    std::vector<std::pair<floatCoordinate*, floatCoordinate> >::iterator iter;
    for(iter = points.begin(); iter != points.end(); ++iter) {
        if(iter->first != NULL) {
            result.push_back(*(iter->first));
        }
    }
    return result;*/
}

/**
 * @brief Patch::trianglesAsVector returns the visible triangles in a vector for easy handling
 * @param viewportType specify viewport on which the frustum culling is performed.
 *        Can be VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ or VIEWPORT_SKELETON.
 */
std::vector<Triangle> Patch::trianglesAsVector(int viewportType) {
    std::vector<Triangle> triangles;

    this->triangles->getAllObjs(triangles);
    if(viewportType == -1) { // return all triangles
        return triangles;
    }
    // filter triangles
    std::vector<Triangle> result;
    std::vector<Triangle>::iterator iter;
    for(iter = triangles.begin(); iter != triangles.end(); ++iter) {
        if(viewportType == VIEWPORT_SKELETON && state->skeletonState->zoomLevel == SKELZOOMMIN) {
            // whole data cube is in frustum
            result.push_back(*iter);
        }
        else if(Renderer::triangleInFrustum(*iter, viewportType)) {
            result.push_back(*iter);
        }
    }
    return result;
}

std::vector<Triangle> Patch::distinguishableTrianglesAsVector(uint viewportType) {
    std::vector<Triangle> triangles;
    switch(viewportType) {
    case VIEWPORT_XY:
    case VIEWPORT_XZ:
    case VIEWPORT_YZ:
        this->distinguishableTrianglesOrthoVP->getAllVisibleObjs(triangles, viewportType);
        break;
    case VIEWPORT_SKELETON:
        this->distinguishableTrianglesSkelVP->getAllVisibleObjs(triangles, viewportType);
        break;
    default:
        qDebug("invalid viewport type");
        break;
    }
    return triangles;
}

/**
 * @brief Patch::clearTriangles clears the 'triangles' octree for recomputation of the surface
 */
void Patch::clearTriangles() {
    delete triangles;

    floatCoordinate center;
    center.x = state->boundary.x/2;
    center.y = state->boundary.y/2;
    center.z = state->boundary.z/2;
    float halfEdgeLength;
    if(center.x > center.y) {
        halfEdgeLength = center.x;
    }
    else {
        halfEdgeLength = center.y;
    }
    if(center.z > halfEdgeLength) {
        halfEdgeLength = center.z;
    }
    triangles = new Octree<Triangle>(center, halfEdgeLength);
    numTriangles = 0;
}

/**
 * @brief Patch::computeTriangles uses the point cloud to compute a surface triangulation
 *        which is stored in the 'triangles' octree.
 * @return returns false if point cloud does not produce a valid surface
 */
bool Patch::computeTriangles() {
    std::vector<floatCoordinate> pointCloud = pointCloudAsVector();
    std::vector<floatCoordinate>::iterator piter;
    pcl_Cloud::Ptr cloud(new pcl_Cloud);

    for(piter = pointCloud.begin(); piter != pointCloud.end(); ++piter) {
        pcl_Point p = pcl_Point(piter->x, piter->y, piter->z);
        cloud->push_back(p);
    }

    pcl_Mesh::Ptr mesh = concaveHull(cloud);
    meshCloud.clear();
    pcl::fromPCLPointCloud2(mesh->cloud, meshCloud);
    this->mesh = *mesh;
    Triangle *triangle;
    for(int tri = 0; tri != mesh->polygons.size(); ++tri) {
        triangle = (Triangle*) malloc(sizeof(Triangle));

        triangle->a.x = meshCloud.points[mesh->polygons[tri].vertices[0]].x;
        triangle->a.y = meshCloud.points[mesh->polygons[tri].vertices[0]].y;
        triangle->a.z = meshCloud.points[mesh->polygons[tri].vertices[0]].z;

        triangle->b.x = meshCloud.points[mesh->polygons[tri].vertices[1]].x;
        triangle->b.y = meshCloud.points[mesh->polygons[tri].vertices[1]].y;
        triangle->b.z = meshCloud.points[mesh->polygons[tri].vertices[1]].z;

        triangle->c.x = meshCloud.points[mesh->polygons[tri].vertices[2]].x;
        triangle->c.y = meshCloud.points[mesh->polygons[tri].vertices[2]].y;
        triangle->c.z = meshCloud.points[mesh->polygons[tri].vertices[2]].z;
        insert(triangle, false);
    }
    updateDistinguishableTriangles();
}

void Patch::recomputeTriangles(floatCoordinate pos, uint halfCubeSize) {
    // clear triangles in the area around pos
    triangles->clearObjsInCube(pos, halfCubeSize);
    std::vector<floatCoordinate> points;
    pointCloud->getObjsInRange(pos, halfCubeSize, points);

    Triangulation triangulation;
    std::vector<floatCoordinate>::iterator piter;
    for(piter = points.begin(); piter != points.end(); ++piter) {
        CGAL_Point p = CGAL_Point(piter->x, piter->y, piter->z);
        triangulation.insert(p);
    }
    // also recompute triangles for points around the border of the cube.
    /*points.clear();
    Triangulation triangulation2;
    pointCloud->getObjsInMargin(pos, halfCubeSize, 100, points);
    for(piter = points.begin(); piter != points.end(); ++piter) {
        CGAL_Point p = CGAL_Point(piter->x, piter->y, piter->z);
        triangulation2.insert(p);
    }*/
    // insert triangles into patches octree
    if(triangulation.is_valid() == false || triangulation.dimension() <= 2) {
        return;
    }
/*
    if(triangulation2.is_valid() == false || triangulation.dimension() <= 2) {
        return;
    }*/
    // iterate over tetrahedrons and extract only the surface triangles
    std::list<CGAL_CellHandle> cells;
    triangulation.incident_cells(triangulation.infinite_vertex(),
                                 std::back_inserter(cells));
    std::list<CGAL_CellHandle>::iterator iter;
    CGAL_Triangle cgalTriangle;
    CGAL_Facet facet;
    Triangle *triangle;
    CGAL_VertexHandle infiniteVertex = triangulation.infinite_vertex();
    for(iter = cells.begin(); iter != cells.end(); ++iter) {
        facet = std::make_pair(*iter, (*iter)->index(infiniteVertex));
        cgalTriangle = triangulation.triangle(facet);
        triangle = (Triangle*) malloc(sizeof(Triangle));
        triangle->a.x = CGAL::to_double(cgalTriangle.vertex(0).x());
        triangle->a.y = CGAL::to_double(cgalTriangle.vertex(0).y());
        triangle->a.z = CGAL::to_double(cgalTriangle.vertex(0).z());

        triangle->b.x = CGAL::to_double(cgalTriangle.vertex(1).x());
        triangle->b.y = CGAL::to_double(cgalTriangle.vertex(1).y());
        triangle->b.z = CGAL::to_double(cgalTriangle.vertex(1).z());

        triangle->c.x = CGAL::to_double(cgalTriangle.vertex(2).x());
        triangle->c.y = CGAL::to_double(cgalTriangle.vertex(2).y());
        triangle->c.z = CGAL::to_double(cgalTriangle.vertex(2).z());
        insert(triangle, true);
    }

  /*  infiniteVertex = triangulation2.infinite_vertex();
    cells.clear();
    triangulation2.incident_cells(triangulation2.infinite_vertex(),
                                 std::back_inserter(cells));
    for(iter = cells.begin(); iter != cells.end(); ++iter) {
        facet = std::make_pair(*iter, (*iter)->index(infiniteVertex));
        cgalTriangle = triangulation2.triangle(facet);
        triangle = (Triangle*) malloc(sizeof(Triangle));
        triangle->a.x = CGAL::to_double(cgalTriangle.vertex(0).x());
        triangle->a.y = CGAL::to_double(cgalTriangle.vertex(0).y());
        triangle->a.z = CGAL::to_double(cgalTriangle.vertex(0).z());

        triangle->b.x = CGAL::to_double(cgalTriangle.vertex(1).x());
        triangle->b.y = CGAL::to_double(cgalTriangle.vertex(1).y());
        triangle->b.z = CGAL::to_double(cgalTriangle.vertex(1).z());

        triangle->c.x = CGAL::to_double(cgalTriangle.vertex(2).x());
        triangle->c.y = CGAL::to_double(cgalTriangle.vertex(2).y());
        triangle->c.z = CGAL::to_double(cgalTriangle.vertex(2).z());
        insert(triangle, true);
    }*/
    updateDistinguishableTriangles();
}

pcl_Mesh::Ptr Patch::concaveHull(pcl_Cloud::Ptr cloudPtr) {
    pcl_ConcaveHull chull;
    chull.setInputCloud (cloudPtr);
    chull.setDimension(3);
    chull.setAlpha(10);
    pcl_Mesh::Ptr mesh(new pcl_Mesh);
    chull.reconstruct(*mesh);
    return mesh;
}

pcl_Mesh::Ptr Patch::marchingCubesSurface(pcl_Cloud::Ptr cloudPtr) {
    double isoLevel = (double) 0.00001; //0 < isoLevel < 1

    // Normal estimation*
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
    pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud (cloudPtr);
    n.setInputCloud (cloudPtr);
    n.setSearchMethod (tree);
   // n.setRadiusSearch (30);
    n.setKSearch (50);
    n.compute (*normals);

    // Concatenate the XYZ and normal fields*
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>);
    concatenateFields(*cloudPtr, *normals, *cloud_with_normals);
    //* cloud_with_normals = cloud + normals

    // Create search tree*
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Init objects
    pcl::PolygonMesh::Ptr mesh(new pcl_Mesh);
    pcl::MarchingCubesHoppe<pcl::PointNormal> mc;
    // Set parameters
    mc.setInputCloud(cloud_with_normals);
    mc.setSearchMethod(tree2);
    mc.setGridResolution(30,30,30);
//  mc.setLeafSize(leafSize);
    mc.setIsoLevel(isoLevel);

    // Reconstruct
    mc.reconstruct(*mesh);
    return mesh;
}

pcl_Mesh::Ptr Patch::poissonReconstruction(pcl_Cloud::Ptr cloudPtr) {
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
    pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloudPtr);
    n.setInputCloud(cloudPtr);
    n.setSearchMethod(tree);
   // n.setRadiusSearch (30);
    n.setKSearch(50);
    n.compute(*normals);
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>);
    concatenateFields(*cloudPtr, *normals, *cloud_with_normals);

    pcl::Poisson<pcl::PointNormal> poisson;
    poisson.setDepth(5);
    poisson.setInputCloud(cloud_with_normals);

    pcl_Mesh::Ptr mesh(new pcl_Mesh);
    poisson.reconstruct(*mesh);
    return mesh;
}

//   functions
/**
 * @brief Patch::newPatch new patches should always be created through this method!
 */
Patch *Patch::newPatch(int patchID) {
    Patch *newPatch = new Patch(NULL, patchID);
    Patch *tmp;
    if(firstPatch == NULL) {
        firstPatch = newPatch;
        firstPatch->next = firstPatch;
        firstPatch->previous = firstPatch;
    }
    else {
        tmp = firstPatch->previous; //firstPatch->previous is really the last patch
        firstPatch->previous = newPatch;
        newPatch->next = firstPatch;
        newPatch->previous = tmp;
        tmp->next = newPatch;
    }
    activePatch = newPatch;
    numPatches++;
    return newPatch;
}

void Patch::delPatch(Patch *patch) {
    if(patch == NULL) {
        return;
    }
    if(numPatches == 1) {
        delete patch;
        activePatch = NULL;
        firstPatch = NULL;
        numPatches--;
        return;
    }
    if(patch->next) {
        patch->next->previous = patch->previous;
    }
    if(patch->previous) {
        patch->previous->next = patch->next;
    }
    if(patch == Patch::activePatch) {
        Patch::activePatch = patch->next;
    }
    if(patch == Patch::firstPatch) {
        Patch::firstPatch = patch->next;
    }
    delete patch;
    numPatches--;
}

Patch *Patch::getPatchWithID(uint patchID) {
    Patch *patch = firstPatch;
    if(patch == NULL) {
        return NULL;
    }
    do {
        if(patch->patchID == patchID) {
            return patch;
        }
        patch = patch->next;
    } while(patch != firstPatch);

    qDebug("No patch with ID %i", patchID);
    return NULL;
}

bool Patch::setActivePatch(uint patchID) {
    Patch *patch = getPatchWithID(patchID);
    if(patch) {
        activePatch = patch;
    }
}

void Patch::delActivePatch() {
    if(Patch::activePatch) {
        delPatch(activePatch);
    }
    /*if(activePatch == NULL) {
        return;
    }
    if(numPatches == 1) {
        delete activePatch;
        activePatch = NULL;
        firstPatch = NULL;
        numPatches--;
        return;
    }
    if(activePatch->next) {
        activePatch->next->previous = activePatch->previous;
    }
    if(activePatch->previous) {
        activePatch->previous->next = activePatch->next;
    }
    Patch *tmp = activePatch;
    activePatch = tmp->next;

    delete tmp;
    numPatches--;*/
}

/**
 * @brief Patch::updateDistinguishableTriangles refills the 'distinguishableTrianglesSkelVP'
 *        and 'distinguishableTrianglesOrthoVP' octrees with triangles that are currently
 *        distinguishable in the corresponding viewport.
 *        Is called on changes to this patch's 'triangles' or the zoom level in the viewports.
 * @param viewportType specify the viewport for which to update the triangles. If not provided,
 *        both octrees for skeleton viewport and ortho viewports are updated.
 */
void Patch::updateDistinguishableTriangles(int viewportType) {
    if(triangles == NULL) {
        return;
    }
    int countOrtho = 0;
    int countSkel = 0;
    switch(viewportType) {
    case(VIEWPORT_SKELETON): // only update for skeleton viewport
        delete distinguishableTrianglesSkelVP;
        distinguishableTrianglesSkelVP = new Octree<Triangle>(*triangles, &countSkel, VIEWPORT_SKELETON);
        break;
    case(VIEWPORT_XY): // only update for ortho viewports
    case(VIEWPORT_XZ):
    case(VIEWPORT_YZ):
        delete distinguishableTrianglesOrthoVP;
        distinguishableTrianglesOrthoVP = new Octree<Triangle>(*triangles, &countOrtho, VIEWPORT_XY);
        break;
    default: // update all
        delete distinguishableTrianglesOrthoVP;
        delete distinguishableTrianglesSkelVP;
        // all ortho viewports have same screen px : data px relation
        distinguishableTrianglesOrthoVP = new Octree<Triangle>(*triangles, &countOrtho, VIEWPORT_XY);
        distinguishableTrianglesSkelVP = new Octree<Triangle>(*triangles, &countSkel, VIEWPORT_SKELETON);
        break;
    }
    //qDebug("ortho: %i, skel: %i", countOrtho, countSkel);
}

void Patch::genRandCloud(uint cloudSize) {
    srand(QTime::currentTime().msec());
    double phi =  0;
    int orient = 1;
    int radius = 500;
    int width = 200;
    int tolerance = 0;
    floatCoordinate *point;

    double r, w, l;

    qDebug("Start creating points on cylinder:");

    for(int i = 0; i < cloudSize; i++)
    {
        point = (floatCoordinate *)malloc(sizeof(floatCoordinate));
        phi = (double) rand()/RAND_MAX * 2 * PI;

        w = width + tolerance * (double) rand()/RAND_MAX;
        r = radius * (double) rand()/RAND_MAX *orient;

        point->x = (float) w * sin(phi);
        point->y = (float) w * cos(phi);
        point->z = (float) r;

        l = sqrt(point->x * point->x + point->y * point->y);

        point->x += state->boundary.x / 2;
        point->y += state->boundary.y / 2;
        point->z += state->boundary.z / 2;
        //Write point to array
        activePatch->insert(point, false);

        tolerance *= (-1);
        orient *= (-1);
    }
    qDebug ("Pointcloud created. NumPoints: %i", activePatch->numPoints);

/*
    qsrand(QTime::currentTime().msec());
    QElapsedTimer timer;
    floatCoordinate *c;
    std::vector<floatCoordinate*> coords;
    std::vector<floatCoordinate*>::iterator iter;
    for(uint i = 0; i < cloudSize; i++) {
        c = new floatCoordinate();
        c->x = rand() % state->boundary.x;
        c->y = rand() % state->boundary.y;
        c->z = rand() % state->boundary.z;

        for(iter = coords.begin(); iter != coords.end(); ++iter) {
            if((*iter)->x == c->x || (*iter)->y == c->y || (*iter)->z == c->z) {
                continue;
            }
        }
        coords.push_back(c);
    }
    qDebug("start insertion");
    timer.start();
    for(iter = coords.begin(); iter != coords.end(); ++iter) {
        Patch::activePatch->insert(*iter, false);
    }
    qDebug("%d points, %d ms", Patch::activePatch->numPoints, timer.elapsed());*/
}

void Patch::genRandTriangulation(uint cloudSize) {
    srand(QTime::currentTime().msec());
    double phi =  0;
    int orient = 1;
    int radius = 500;
    int width = 200;
    int tolerance = 0;
    floatCoordinate *point;

    double r, w, l;

    qDebug("Start creating points on cylinder:");

    for(int i = 0; i < cloudSize; i++) {
        point = (floatCoordinate *)malloc(sizeof(floatCoordinate));
        phi = (double) rand()/RAND_MAX * 2 * PI;

        w = width + tolerance * (double) rand()/RAND_MAX;
        r = radius * (double) rand()/RAND_MAX *orient;

        point->x = (float) w * sin(phi);
        point->y = (float) w * cos(phi);
        point->z = (float) r;

        l = sqrt(point->x * point->x + point->y * point->y);

        point->x += state->boundary.x / 2;
        point->y += state->boundary.y / 2;
        point->z += state->boundary.z / 2;
        //Write point to array
        if(activePatch->insert(point, false)) {
            activePatch->recomputeTriangles(*point, 50);
        }

        tolerance *= (-1);
        orient *= (-1);
    }
    Patch::activePatch->updateDistinguishableTriangles();
    qDebug ("Pointcloud created. NumPoints: %i", activePatch->numPoints);
}

// normal of point p as angle bisector of the two lines through p and its left and right neighbour
// todo: handle case, where loop isn't closed
void Patch::computeNormals() {
    if(Patch::activeLoop.size() == 0) {
        qDebug("no points in loop to compute normals.");
        return;
    }

    floatCoordinate p, left, right, q, direct1, direct2, normal;
    p = activeLoop[0];
    left = activeLoop[activeLoop.size()-1];
    right = activeLoop[1];
    SUB_ASSIGN_COORDINATE(direct1, p, left);
    SUB_ASSIGN_COORDINATE(direct2, p, right);
    SET_COORDINATE(q, p.x + direct1.x + direct2.x, p.y + direct1.y + direct2.y, p.z + direct1.z + direct2.z);
    SET_COORDINATE(normal, p.x - q.x, p.y - q.y, p.z - q.z);
    normalizeVector(&normal);

}

void Patch::visiblePoints(uint viewportType) {
    if(viewportType != VIEWPORT_XY and viewportType != VIEWPORT_XZ and
       viewportType != VIEWPORT_YZ and viewportType != VIEWPORT_ARBITRARY and viewportType != VIEWPORT_SKELETON) {
        qDebug("invalid viewport type: %i", viewportType);
        return;
    }
    if(Patch::activePatch == NULL) {
        qDebug("no active patch.");
        return;
    }
    if(Patch::activePatch->meshCloud.points.size() == 0) {
        qDebug("no points.");
        return;
    }

    pcl::octree::OctreePointCloudSearch<pcl::PointXYZ> octree (1.f);
    pcl_Cloud::Ptr cloud(&Patch::activePatch->meshCloud);
    octree.setInputCloud(cloud);
    octree.addPointsFromInputCloud();

    Coordinate upperLeft, lowerRight;
    switch(viewportType) {
    case VIEWPORT_XZ:
        SET_COORDINATE(upperLeft, state->viewerState->vpConfigs[VIEWPORT_XZ].leftUpperDataPxOnScreen.x,
                                  state->viewerState->vpConfigs[VIEWPORT_XZ].leftUpperDataPxOnScreen.z,
                                  state->viewerState->vpConfigs[VIEWPORT_XZ].leftUpperDataPxOnScreen.y);
        break;
    case VIEWPORT_YZ:
        SET_COORDINATE(upperLeft, state->viewerState->vpConfigs[VIEWPORT_XZ].leftUpperDataPxOnScreen.y,
                                  state->viewerState->vpConfigs[VIEWPORT_XZ].leftUpperDataPxOnScreen.z,
                                  state->viewerState->vpConfigs[VIEWPORT_XZ].leftUpperDataPxOnScreen.x);
        break;
    case VIEWPORT_XY:
    default:
        SET_COORDINATE(upperLeft, state->viewerState->vpConfigs[VIEWPORT_XY].leftUpperDataPxOnScreen.x,
                                  state->viewerState->vpConfigs[VIEWPORT_XY].leftUpperDataPxOnScreen.y,
                                  state->viewerState->vpConfigs[VIEWPORT_XY].leftUpperDataPxOnScreen.z);
    }
    SET_COORDINATE(lowerRight,
                        upperLeft.x
                               + state->viewerState->vpConfigs[VIEWPORT_XZ].edgeLength
                               * 1/state->viewerState->vpConfigs[VIEWPORT_XZ].screenPxXPerDataPx,
                        upperLeft.y
                               + state->viewerState->vpConfigs[VIEWPORT_XZ].edgeLength
                               * 1/state->viewerState->vpConfigs[VIEWPORT_XZ].screenPxYPerDataPx,
                        upperLeft.z);

    if(state->viewerState->defaultTexData == NULL) {
        LOG("Out of memory.")
        _Exit(false);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(Byte)
                                                     * 3);
    Eigen::Vector3f origin, direction;
    pcl::octree::OctreePointCloudVoxelCentroid<pcl::PointXYZ>::AlignedPointTVector voxelCenters;
    floatCoordinate voxel;
    int minX, maxX;
    for(int y = upperLeft.y; y <= lowerRight.y; ++y) {
        origin(0) = (float)upperLeft.x; origin(1) = (float)y; origin(2) = (float)upperLeft.z;
        direction(0) = (float)lowerRight.x - origin(0); direction(1) = 0.; direction(2) = 0.;
        octree.getIntersectedVoxelCenters(origin, direction, voxelCenters, 2);
        qDebug("found voxels for %.1f, %.1f, %.1f with dir %.1f: ", origin(0), origin(1), origin(2), direction(0));
        if(voxelCenters.size() > 0) { // ray intersected surface
            minX = maxX = voxelCenters[0].x;
            for(uint i = 1; i < voxelCenters.size(); ++i) {
                if(voxelCenters[i].x < minX) {
                    minX = voxelCenters[i].x;
                }
                if(voxelCenters[i].x > maxX) {
                    maxX = voxelCenters[i].x;
                }
                voxel.x = voxelCenters[i].x;
                voxel.y = voxelCenters[i].y;
                voxel.z = voxelCenters[i].z;
                qDebug("%.2f, %.2f, %.2f", voxel.x, voxel.y, voxel.z);
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, Patch::texHandle);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 state->viewerState->vpConfigs[viewportType].texture.edgeLengthPx,
                 state->viewerState->vpConfigs[viewportType].texture.edgeLengthPx,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 Patch::activePatch->texData);
    glBindTexture(GL_TEXTURE_2D, 0); // unbind texhandle

//    Coordinate upperLeft;
//    SET_COORDINATE(upperLeft, state->viewerState->vpConfigs[viewportType].leftUpperDataPxOnScreen.x,
//                              state->viewerState->vpConfigs[viewportType].leftUpperDataPxOnScreen.y,
//                              state->viewerState->vpConfigs[viewportType].leftUpperDataPxOnScreen.z);
//    Coordinate lowerRight;
//    SET_COORDINATE(lowerRight,
//                    (upperLeft.x
//                           + state->viewerState->vpConfigs[viewportType].edgeLength
//                           * 1/state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx),
//                    (upperLeft.y
//                           + state->viewerState->vpConfigs[viewportType].edgeLength
//                           * 1/state->viewerState->vpConfigs[viewportType].screenPxYPerDataPx),
//                    upperLeft.z);
//    Eigen::Vector3f min(state->viewerState->vpConfigs[VIEWPORT_XY].leftUpperDataPxOnScreen.x,
//                        state->viewerState->vpConfigs[VIEWPORT_XY].leftUpperDataPxOnScreen.y,
//                        state->viewerState->vpConfigs[VIEWPORT_XY].leftUpperDataPxOnScreen.z);
//    Eigen::Vector3f max(lowerRight.x, lowerRight.y, lowerRight.z);
//    std::vector<int> indices;
//    octree.boxSearch(min, max, indices);
//    int minX = cloud->points[0].x;
//    int maxX = cloud->points[0].x;
//    int minY = cloud->points[0].y;
//    int maxY = cloud->points[0].y;
//    for(int i = 0; i < indices.size(); ++i) {
//        if(cloud->points[i].x < minX) {
//            minX = cloud->points[i].x;
//        }
//        if(cloud->points[i].x > maxX) {
//            maxX = cloud->points[i].x;
//        }
//        if(cloud->points[i].y < minY) {
//            minY = cloud->points[i].y;
//        }
//        if(cloud->points[i].y > maxY) {
//            maxY = cloud->points[i].y;
//        }
//        result.push_back(cloud->points[i]);
//       // qDebug("%i", indices[i]);
//    }

//    int z = state->viewerState->currentPosition.z;
//    Coordinate testPoint;
//    for(int y = minY; y <= maxY; ++y) {
//        for(int x = minX; x <= maxX; ++x) {
//            SET_COORDINATE(testPoint, x, y, z);
//        }
//    }

}
