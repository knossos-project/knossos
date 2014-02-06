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
float Patch::voxelPerPoint = .3;
std::vector<floatCoordinate> Patch::activeLoop;
std::vector<floatCoordinate> Patch::activeLine;
std::vector<std::vector<floatCoordinate> > Patch::lineBuffer;
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
    loops = new Octree<PatchLoop*>(center, halfEdgeLength);
    distinguishableTrianglesOrthoVP = new Octree<Triangle>(center, halfEdgeLength);
    distinguishableTrianglesSkelVP = new Octree<Triangle>(center, halfEdgeLength);
    setTree(state->skeletonState->activeTree);

    //texData = (Byte *)malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 4);
    memset(texData, 0, sizeof(texData)); // default is a empty texture
}


Patch::~Patch() {
    std::vector<PatchLoop *> loopVec;
    loops->getAllObjs(loopVec);
    for(uint i = 0; i < loopVec.size(); ++i) {
        delete loopVec[i];
    }
    delete loops;
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
    return !pointCloud->objInRange(point, voxelPerPoint);
}

/**
 * @brief Patch::addInterpolatedPoint adds a point on the line between p and q with a distance of 'voxelPerPoint' from p
 * @return the interpolated point
 */
void Patch::addInterpolatedPoint(floatCoordinate p, floatCoordinate q) {
    floatCoordinate v;
    SUB_ASSIGN_COORDINATE(v, q, p);
    normalizeVector(&v);

    floatCoordinate point;
    SET_COORDINATE(point, p.x + voxelPerPoint*v.x, p.y + voxelPerPoint*v.y, p.z + voxelPerPoint*v.z);
    pointCloud->insert(point, point, false);
    activeLine.push_back(point);
    numPoints++;
    if(q.x - point.x < -voxelPerPoint or q.x - point.x > voxelPerPoint
            or q.y - point.y < -voxelPerPoint or q.y - point.y > voxelPerPoint
            or q.z - point.z < -voxelPerPoint or q.z - point.z > voxelPerPoint) {
        addInterpolatedPoint(point, q);
    }
}

/**
 * @brief Patch::insert insert a new triangle to this patch's surface
 * @param triangle the new triangle
 * @param replace define behaviour if a triangle at same location already exists: replace, or do nothing
 * @return true on insertion, false otherwise
 */
bool Patch::insert(Triangle triangle, bool replace) {
    if(triangles->insert(triangle, centroidTriangle(triangle), replace)) {
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
bool Patch::insert(floatCoordinate point, bool replace) {
    if(allowPoint(point) == false) { // point too close to last point
        return false;
    }
    // if point is too far away from last point add interpolated points to fill the distance
    if(activeLine.size() > 0) {
        floatCoordinate p = activeLine.back();
        if(point.x - p.x < -voxelPerPoint or point.x - p.x > voxelPerPoint
              or point.y - p.y < -voxelPerPoint or point.y - p.y > voxelPerPoint
              or point.z - p.z < -voxelPerPoint or point.z - p.z > voxelPerPoint) {
            addInterpolatedPoint(p, point);
            newPoints = true;
        }
    }
    if(pointCloud->insert(point, point, replace)) {
        numPoints++;
        activeLine.push_back(point);
        newPoints = true;
    }
    return newPoints;
}

bool Patch::insert(PatchLoop *loop, uint viewportType) {
    if(loop) {
        loops->insert(loop, centroidPolygon(loop->points), true);
       // computeVolume(viewportType, loop);
    }
}

bool Patch::addLineToLoop(int viewportType) {
    if(activeLine.size() == 0) {
        return false;
    }
    activeLoop.insert(activeLoop.end(), activeLine.begin(), activeLine.end());
    lineBuffer.push_back(activeLine);
    activeLine.clear();
}

/**
 * @brief Patch::alignToLine align start of new line to active line.
 * @param point the start point of the new line
 * @param index index of the line to which the new line is to be aligned
 * @param startPoint flag that tells, wether it should be aligned to start point or end point
 */
void Patch::alignToLine(floatCoordinate point, int index, bool startPoint) {
    if(startPoint) {
        std::reverse(lineBuffer[index].begin(), lineBuffer[index].end());
    }
    activeLine = lineBuffer[index];
    lineBuffer.erase(lineBuffer.begin() + index);

    activePatch->addInterpolatedPoint(activeLine.back(), point);
}


/**
 * @brief Patch::lineFinished decides what to do with the finished line:
 *              connect it to an existing line, if they touch at the end points.
 *              If the finished line closes the loop, a new 'PatchLoop' is created and added to the patch
 * @param lastPoint the last point of the finished line
 * @param viewportType the viewport in which the line was drawn (XY, XZ, YZ)
 */
void Patch::lineFinished(floatCoordinate lastPoint, int viewportType) {
    if(activePatch == NULL) {
        return;
    }
    floatCoordinate distanceVec, snapPoint;
    float distance = INT_MAX;
    SET_COORDINATE(snapPoint, -1, -1, -1);
    if(lineBuffer.size() > 0) {
        for(uint i = 0; i < lineBuffer.size(); ++i) {
            SUB_ASSIGN_COORDINATE(distanceVec, lineBuffer[i][0], lastPoint);
            if((distance = euclidicNorm(&distanceVec)) < AUTO_ALIGN_RADIUS) {
                snapPoint = lineBuffer[i][0];
            }
            else {
                SUB_ASSIGN_COORDINATE(distanceVec, lineBuffer[i][lineBuffer[i].size() - 1],
                                                    lastPoint);
                if((distance = euclidicNorm(&distanceVec)) < AUTO_ALIGN_RADIUS) {
                    snapPoint = lineBuffer[i][lineBuffer[i].size() - 1];
                    std::reverse(lineBuffer[i].begin(), lineBuffer[i].end());
                }
            }
            if(distance < AUTO_ALIGN_RADIUS) {
                // found point to snap to. Fill distance with interpolated points,
                // then connect the two lines
                if(lastPoint.x - snapPoint.x < -voxelPerPoint or lastPoint.x - snapPoint.x > voxelPerPoint
                        or lastPoint.y - snapPoint.y < -voxelPerPoint or lastPoint.y - snapPoint.y > voxelPerPoint
                        or lastPoint.z - snapPoint.z < -voxelPerPoint or lastPoint.z - snapPoint.z > voxelPerPoint) {
                    activePatch->addInterpolatedPoint(lastPoint, snapPoint);
                }
                activeLine.insert(activeLine.end(), lineBuffer[i].begin(), lineBuffer[i].end());
                lineBuffer.erase(lineBuffer.begin() + i);
                lineBuffer.push_back(activeLine);
                break;
            }
        }
        if(snapPoint.x == -1) { // no point found to snap to, so simply add this line to the line buffer
            lineBuffer.push_back(activeLine);
        }
    }
    else { // no other lines to check against, check if this single line is already closed
        SUB_ASSIGN_COORDINATE(distanceVec, activeLine[0], lastPoint);
        if((distance = euclidicNorm(&distanceVec)) < AUTO_ALIGN_RADIUS) {
            activePatch->addInterpolatedPoint(lastPoint, activeLine[0]);
        }
        else {
            lineBuffer.push_back(activeLine);
        }
    }

    if(lineBuffer.size() == 0) {
        // loop is closed now
        PatchLoop *newLoop = new PatchLoop(activeLine);
        activePatch->insert(newLoop, viewportType);
    }
    activeLine.clear();
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
    std::vector<std::pair<floatCoordinate*, floatCoordinate1> >::iterator iter;
    for(iter = points.begin(); iter != points.end(); ++iter) {
        if(iter->first != NULL) {
            result.push_back(*(iter->first));
        }
    }
    return result;*/
}

/**
 * @brief Patch::loopsAsVector returns the patch's loops as vector for easy handling
 * @param viewportType specifies a viewport to only retrieve the visible loops in this viewport
 * @return the vector of loops
 */
std::vector<PatchLoop*> Patch::loopsAsVector(int viewportType) {
    std::vector<PatchLoop*> loopVector;
    if(viewportType == -1) {
        loops->getAllObjs(loopVector);
    }
    else {
        loops->getAllVisibleObjs(loopVector, viewportType);
    }
    return loopVector;
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
    Triangle triangle;
    for(int tri = 0; tri != mesh->polygons.size(); ++tri) {
        triangle.a.x = meshCloud.points[mesh->polygons[tri].vertices[0]].x;
        triangle.a.y = meshCloud.points[mesh->polygons[tri].vertices[0]].y;
        triangle.a.z = meshCloud.points[mesh->polygons[tri].vertices[0]].z;

        triangle.b.x = meshCloud.points[mesh->polygons[tri].vertices[1]].x;
        triangle.b.y = meshCloud.points[mesh->polygons[tri].vertices[1]].y;
        triangle.b.z = meshCloud.points[mesh->polygons[tri].vertices[1]].z;

        triangle.c.x = meshCloud.points[mesh->polygons[tri].vertices[2]].x;
        triangle.c.y = meshCloud.points[mesh->polygons[tri].vertices[2]].y;
        triangle.c.z = meshCloud.points[mesh->polygons[tri].vertices[2]].z;
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
    Triangle triangle;
    CGAL_VertexHandle infiniteVertex = triangulation.infinite_vertex();
    for(iter = cells.begin(); iter != cells.end(); ++iter) {
        facet = std::make_pair(*iter, (*iter)->index(infiniteVertex));
        cgalTriangle = triangulation.triangle(facet);
        triangle.a.x = CGAL::to_double(cgalTriangle.vertex(0).x());
        triangle.a.y = CGAL::to_double(cgalTriangle.vertex(0).y());
        triangle.a.z = CGAL::to_double(cgalTriangle.vertex(0).z());

        triangle.b.x = CGAL::to_double(cgalTriangle.vertex(1).x());
        triangle.b.y = CGAL::to_double(cgalTriangle.vertex(1).y());
        triangle.b.z = CGAL::to_double(cgalTriangle.vertex(1).z());

        triangle.c.x = CGAL::to_double(cgalTriangle.vertex(2).x());
        triangle.c.y = CGAL::to_double(cgalTriangle.vertex(2).y());
        triangle.c.z = CGAL::to_double(cgalTriangle.vertex(2).z());
        insert(triangle, true);
    }

  /*  infiniteVertex = triangulation2.infinite_vertex();
    cells.clear();
    triangulation2.incident_cells(triangulation2.infinite_vertex(),
                                 std::back_inserter(cells));
    for(iter = cells.begin(); iter != cells.end(); ++iter) {
        facet = std::make_pair(*iter, (*iter)->index(infiniteVertex));
        cgalTriangle = triangulation2.triangle(facet);
        triangle = (Triangle*) malloc( sizeof(Triangle));
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

/**
 * @brief Patch::computeVolume computes visible volume inside given viewport by iterating over y-direction of the viewport
 *        and ray casting in x direction.
 *        Works without normals, therefore only for volumes smaller than the super cube!
 * @param currentVP the viewport for which to display a volume
 */
void Patch::computeVolume(int currentVP, PatchLoop *loop) {
    Coordinate lu, rl; // left upper and right lower data coordinate
    vpConfig *vp = &state->viewerState->vpConfigs[currentVP];

    switch(currentVP) {
    case VIEWPORT_XY:
        lu.x = vp->leftUpperDataPxOnScreen.x;
        lu.y = vp->leftUpperDataPxOnScreen.y;
        lu.z = vp->leftUpperDataPxOnScreen.z;
        break;
    case VIEWPORT_XZ:
        lu.x = vp->leftUpperDataPxOnScreen.x;
        lu.y = vp->leftUpperDataPxOnScreen.z;
        lu.z = vp->leftUpperDataPxOnScreen.y;
        break;
    case VIEWPORT_YZ:
        lu.x = vp->leftUpperDataPxOnScreen.y;
        lu.y = vp->leftUpperDataPxOnScreen.z;
        lu.z = vp->leftUpperDataPxOnScreen.x;
        break;

    }

    SET_COORDINATE(rl, lu.x + vp->edgeLength/vp->screenPxXPerDataPx,
                       lu.y + vp->edgeLength/vp->screenPxYPerDataPx,
                       lu.z);

    float edgeLengthXInDataPx = vp->edgeLength/vp->screenPxXPerDataPx;
    float edgeLengthYInDataPx = vp->edgeLength/vp->screenPxYPerDataPx;

    int vpDataPixels[(int)ceil(edgeLengthYInDataPx)][(int)ceil(edgeLengthXInDataPx)];
    memset(vpDataPixels, 0, sizeof(vpDataPixels));
    std::vector<floatCoordinate> results; // found points on one ray
    std::vector<floatCoordinate> boundingPoints; // filtered results
    floatCoordinate p, q; // pq is any pair of boundingPoints enclosing the volume
    switch(currentVP) {
    case VIEWPORT_XY:
    {
        // first a horizontal ray cast
        for(int y = lu.y; y <= rl.y; ++y) {
            results = pointsOnLine(loop, -1, y, lu.z);
            if(results.size() == 0) {
                continue;
            }
            if(results.size() > 2) {
                // identify all bounding points, i.e.
                // - the left most and the right most point
                // - points with distance of at least one voxel from other points
                // then add them to the bounding points in ascending order (of x-coordinates) for correct pairing

                // first find the left-most and right-most point
                float minX = INT_MAX, maxX = 0;
                std::vector<int> skipIndices;
                int minIndex, maxIndex;
                for(uint i = 0; i < results.size(); ++i) {
                    if(minX > results[i].x) {
                        minX = results[i].x;
                        minIndex = i;
                    }
                    else if(maxX < results[i].x) {
                        maxX = results[i].x;
                        maxIndex = i;
                    }
                }
                boundingPoints.push_back(results[minIndex]);
                // now find points with distance of at least one voxel from other points
                for(uint i = 0; i <  results.size(); ++i) {
                    if(std::find(skipIndices.begin(), skipIndices.end(), i) != skipIndices.end()) {
                        continue;
                    }

                    for(uint j = 0; j < results.size(); ++j) {
                        if(i == j) {
                            continue;
                        }
                        if(results[j].x - results[i].x > -2 and results[j].x - results[i].x < 2) {
                            if(j == minIndex or j == maxIndex) {
                                skipIndices.push_back(i); // min and max always stay in the result
                            }
                            else {
                                skipIndices.push_back(j);
                            }
                        }
                    }
                    if(i != maxIndex and i != minIndex and std::find(skipIndices.begin(), skipIndices.end(), i)
                            == skipIndices.end()) {
                        // point has enough distance to any other point
                        // ensure ascending order
                        if(boundingPoints[boundingPoints.size() - 1].x > results[i].x) {
                            floatCoordinate tmp = boundingPoints[boundingPoints.size() - 1];
                            boundingPoints.pop_back();
                            boundingPoints.push_back(results[i]);
                            boundingPoints.push_back(tmp);
                        }
                        else {
                            boundingPoints.push_back(results[i]);
                        }
                    }
                }
                boundingPoints.push_back(results[maxIndex]);
            }

            // if not even number of pairs, something went wrong. Then we'll only consider the first and last point
            // and vertical ray cast will fix it
            int maxPair = (boundingPoints.size() % 2 == 0)? boundingPoints.size()/2 : 1;
            int endCond = (maxPair == 1)? 1 : boundingPoints.size();
            for(int pair = 0; pair < endCond; pair += 2) {
                // fill texture stripe with patch color
                if(maxPair == 1) {
                    p = boundingPoints[0];
                    q = boundingPoints[boundingPoints.size() -1];
                }
                else {
                    p = boundingPoints[pair];
                    q = boundingPoints[pair + 1];
                }
                for(int i = roundFloat(p.x) - lu.x; i < roundFloat(q.x) - lu.x; ++i) {
                    vpDataPixels[y - lu.y][i] += 1;
                }
            }
            // clear for next ray
            boundingPoints.clear();
            results.clear();
        }

        // now start vertical ray casting
        for(int x = lu.x; x <= rl.x; ++x) {
            results = pointsOnLine(loop, x, -1, lu.z);
            if(results.size() == 0) {
                continue;
            }
            if(results.size() > 2) {
                // identify all bounding points, i.e.
                // - the left most and the right most point
                // - points with distance of at least one voxel from other points
                // then add them to the bounding points in ascending order (of x-coordinates) for correct pairing

                // first find the left-most and right-most point
                float minY = INT_MAX, maxY = 0;
                std::vector<int> skipIndices;
                int minIndex, maxIndex;
                for(int i = 0; i < results.size(); ++i) {
                    if(minY > results[i].y) {
                        minY = results[i].y;
                        minIndex = i;
                    }
                    else if(maxY < results[i].y) {
                        maxY = results[i].y;
                        maxIndex = i;
                    }
                }
                boundingPoints.push_back(results[minIndex]);
                // now find points with distance of at least one voxel from other points
                for(uint i = 0; i <  results.size(); ++i) {
                    if(std::find(skipIndices.begin(), skipIndices.end(), i) != skipIndices.end()) {
                        continue;
                    }

                    for(uint j = 0; j < results.size(); ++j) {
                        if(i == j) {
                            continue;
                        }
                        if(results[j].y - results[i].y > -2 and results[j].y - results[i].y < 2) {
                            if(j == minIndex or j == maxIndex) {
                                skipIndices.push_back(i); // min and max always stay in the result
                            }
                            else {
                                skipIndices.push_back(j);
                            }
                        }
                    }
                    if(i != maxIndex and i != minIndex and std::find(skipIndices.begin(), skipIndices.end(), i)
                            == skipIndices.end()) {
                        // point has enough distance to any other point
                        // ensure ascending order
                        if(boundingPoints[boundingPoints.size() - 1].y > results[i].y) {
                            floatCoordinate tmp = boundingPoints[boundingPoints.size() - 1];
                            boundingPoints.pop_back();
                            boundingPoints.push_back(results[i]);
                            boundingPoints.push_back(tmp);
                        }
                        else {
                            boundingPoints.push_back(results[i]);
                        }
                    }
                }
                boundingPoints.push_back(results[maxIndex]);
            }

            // if not even number of pairs, something went wrong. Then we'll only consider the first and last point
            // and vertical ray cast will fix it
            int maxPair = (boundingPoints.size() % 2 == 0)? boundingPoints.size()/2 : 1;
            int endCond = (maxPair == 1)? 1 : boundingPoints.size();
            for(int pair = 0; pair < endCond; pair += 2) {
                // fill texture stripe with patch color
                if(maxPair == 1) {
                    p = boundingPoints[0];
                    q = boundingPoints[boundingPoints.size() -1];
                }
                else {
                    p = boundingPoints[pair];
                    q = boundingPoints[pair + 1];
                }

                for(int i = roundFloat(p.y) - lu.y; i < roundFloat(q.y) - lu.y; ++i) {
                    vpDataPixels[i][x - lu.x] += 1;
                }
            }
            // clear for next ray
            boundingPoints.clear();
            results.clear();
        }

        // now reduce to intersection of both ray castings and compress result to start end end point of each stripe
        Coordinate start, end;
        int found = 0;
        for(int row = 0; row < ceil(edgeLengthYInDataPx); ++row) {
            for(int col = 0; col < ceil(edgeLengthXInDataPx); ++col) {
                if(vpDataPixels[row][col] == 2) {
                    if(found == 0) {
                        found++; //start of new stripe
                        SET_COORDINATE(start, lu.x + col, lu.y + row, lu.z);
                    }
                }
                else if(found == 1) { // start point was found, this is one pixel after end point
                    SET_COORDINATE(end, lu.x + col - 1, lu.y + row, lu.z);
                    loop->volumeStripes.push_back(std::pair<Coordinate, Coordinate>(start, end));
                    found = 0;
                }
            }
            if(found == 1) {
            // only a start point was found in the last row. so volume stripe must go from start point to end of row
                SET_COORDINATE(end, lu.x + (int)ceil(edgeLengthXInDataPx), lu.y + row, lu.z);
                loop->volumeStripes.push_back(std::pair<Coordinate, Coordinate>(start, end));
                found = 0;
            }
        }
        break;
    }
    case VIEWPORT_XZ:
//        for(int z = lu.z; z <= rl.z; ++z) {
//            pointCloud->getObjsOnLine(results, -1, lu.y, z);
//            if(results.size() == 0) {
//                continue;
//            }
//            if(results.size() >= 2) {
//                // fill texture stripe with patch color
//                p = results[0];
//                q = results[results.size()-1];
//                // translate dataset (0, 0) (upper left) to openGL (0, 0) (lower left)
//                y_offset = roundFloat(subTextureRU.y - dataPxYPerTexel * (z - lu.z));
//                for(int x = p.x; x <= q.x; ++x) {
//                    x_offset = roundFloat(subTextureLL.x + (x - lu.x) * dataPxXPerTexel);
//                    for(int texelX = 0; texelX < dataPxXPerTexel; ++texelX) {
//                        texData[y_offset][x_offset + texelX][0] = 255 * correspondingTree->color.r;
//                        texData[y_offset][x_offset + texelX][1] = 255 * correspondingTree->color.g;
//                        texData[y_offset][x_offset + texelX][2] = 255 * correspondingTree->color.b;
//                        texData[y_offset][x_offset + texelX][3] = 255 * correspondingTree->color.a;
//                        for(int texelY = 0; texelY < dataPxYPerTexel; ++texelY) {
//                            texData[y_offset + texelY][x_offset + texelX][0] = 255 * correspondingTree->color.r;
//                            texData[y_offset + texelY][x_offset + texelX][1] = 255 * correspondingTree->color.g;
//                            texData[y_offset + texelY][x_offset + texelX][2] = 255 * correspondingTree->color.b;
//                            texData[y_offset + texelY][x_offset + texelX][3] = 255 * correspondingTree->color.a;
//                        }
//                    }
//                }
//            }
//            results.clear();
//        }
        break;
    case VIEWPORT_YZ:
//        for(int z = lu.z; z <= rl.z; ++z) {
//            pointCloud->getObjsOnLine(results, lu.x, -1, z);
//            if(results.size() == 0) {
//                continue;
//            }
//            if(results.size() >= 2) {
//                // fill texture stripe with patch color
//                p = results[0];
//                q = results[results.size()-1];
//                // translate dataset (0, 0) (upper left) to openGL (0, 0) (lower left)
//                y_offset = roundFloat(subTextureRU.y - dataPxYPerTexel * (z - lu.z));
//                for(int y = p.y; y <= q.y; ++y) {
//                    x_offset = roundFloat(subTextureLL.x + (y - lu.y) * dataPxXPerTexel);
//                    for(int texelX = 0; texelX < dataPxXPerTexel; ++texelX) {
//                        texData[y_offset][x_offset + texelX][0] = 255 * correspondingTree->color.r;
//                        texData[y_offset][x_offset + texelX][1] = 255 * correspondingTree->color.g;
//                        texData[y_offset][x_offset + texelX][2] = 255 * correspondingTree->color.b;
//                        texData[y_offset][x_offset + texelX][3] = 255 * correspondingTree->color.a;
//                        for(int texelY = 0; texelY < dataPxYPerTexel; ++texelY) {
//                            texData[y_offset + texelY][x_offset + texelX][0] = 255 * correspondingTree->color.r;
//                            texData[y_offset + texelY][x_offset + texelX][1] = 255 * correspondingTree->color.g;
//                            texData[y_offset + texelY][x_offset + texelX][2] = 255 * correspondingTree->color.b;
//                            texData[y_offset + texelY][x_offset + texelX][3] = 255 * correspondingTree->color.a;
//                        }
//                    }
//                }
//            }
//            results.clear();
//        }
        break;
    }
}

/**
 * @brief Patch::pointsOnLine get all points of given loop that lie on the line specified by two coordinates.
 *        Set third coordinate to -1.
 *        A point lies on the line, if its floored coordinates equal the line coordinates.
 * @param loop the loop whose points are tested
 * @return the vector of points lying on defined line.
 */
std::vector<floatCoordinate> Patch::pointsOnLine(PatchLoop *loop, int x, int y, int z) {
    std::vector<floatCoordinate> results;
    if(loop == NULL) {
        return results;
    }

    for(uint i = 0; i < loop->points.size(); ++i) {
        if(z == -1) {
            if(floor(loop->points[i].x) == x and floor(loop->points[i].y) == y) {
                results.push_back(loop->points[i]);
            }
        }
        else if(y == -1) {
            if(floor(loop->points[i].x) == x and floor(loop->points[i].z) == z) {
                results.push_back(loop->points[i]);
            }
        }
        else if(x == -1) {
            if(floor(loop->points[i].y) == y and floor(loop->points[i].z) == z) {
                results.push_back(loop->points[i]);
            }
        }
    }
    return results;
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
    floatCoordinate point;

    double r, w, l;

    qDebug("Start creating points on cylinder:");

    for(int i = 0; i < cloudSize; i++)
    {
        phi = (double) rand()/RAND_MAX * 2 * PI;

        w = width + tolerance * (double) rand()/RAND_MAX;
        r = radius * (double) rand()/RAND_MAX *orient;

        point.x = (float) w * sin(phi);
        point.y = (float) w * cos(phi);
        point.z = (float) r;

        l = sqrt(point.x * point.x + point.y * point.y);

        point.x += state->boundary.x / 2;
        point.y += state->boundary.y / 2;
        point.z += state->boundary.z / 2;
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
    floatCoordinate point;

    double r, w, l;

    qDebug("Start creating points on cylinder:");

    for(int i = 0; i < cloudSize; i++) {
        phi = (double) rand()/RAND_MAX * 2 * PI;

        w = width + tolerance * (double) rand()/RAND_MAX;
        r = radius * (double) rand()/RAND_MAX *orient;

        point.x = (float) w * sin(phi);
        point.y = (float) w * cos(phi);
        point.z = (float) r;

        l = sqrt(point.x * point.x + point.y * point.y);

        point.x += state->boundary.x / 2;
        point.y += state->boundary.y / 2;
        point.z += state->boundary.z / 2;
        //Write point to array
        if(activePatch->insert(point, false)) {
            activePatch->recomputeTriangles(point, 50);
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
