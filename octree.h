#ifndef OCTREE_H
#define OCTREE_H

#include <math.h>

#include "knossos-global.h"
#include "functions.h"
#include "renderer.h"

/**
 * The Octree template class.
 * Each node consumes 64 byte in a 32-bit application and 100 byte in a 64-bit.
 *
 * - 'object' is the data stored in this octree
 * - 'point' holds the position of the object. Any sub-octree with 'point'.{x, y, z} == -1 has no object set
 * - 'children' are the child octants of each octree and are again octrees. Order of children:
 *        ____________
 *       /|    /     /|
 *      /_|1__/|_5__/ |
 *     /| |  /||   /| |
 *    /_3_|_/_||7_/_|_|
 *    | |/  | |/  | | /
 *    | /___|_|___|_|/|
 *    |/    |/    | / |
 *    |_____|_____|/|_|
 *    | |/  | |/  | | /
 *    | /_0_|_|_4_|_|/
 *    |/    |/    | /
 *    |_2___|__6__|/
 *
 * - a sub-octree is a leaf if all its children are NULL. It does NOT necessarily hold an object.
 * - sub-octrees with children, i.e. internal nodes can also hold objects.
 */

extern struct stateInfo *state;

template<typename T> class Octree {
public:
    static std::vector<T> dummyVec; //! default parameter for methods expecting a vector.

    floatCoordinate center; //! The physical center of this node
    float halfEdgeLen;  //! Half the width/height/depth of this node

    Octree *children[8]; //! Pointers to child octants
    std::vector<T> objects;   //! the stored objects
    std::vector<floatCoordinate> positions; //! object positions. Indices are synchronous with 'objects' vector

    Octree(const floatCoordinate center, const float halfEdgeLen)
        : center(center), halfEdgeLen(halfEdgeLen) {
        // Initially, there are no children
        for(int i=0; i<8; ++i) {
            children[i] = NULL;
        }
    }

    /**
     * @brief Octree copy constructor for deep copy of octree and all its content.
     *        The type of objects in this octree must provide a copy constructor as well.
     * @param copy the octree to be copied
     * @param viewportType alternatively specify a viewportType to copy only objects
     *        that are distinguishable in that viewport depending on the zoom level.
     *        If not provided, the whole octree is copied.
     *        A sub-octree with cube size <= 1 screenpixel will not be traversed,
     *        but a virtual triangle will be created to fill this pixel:
     *       _____________
     *      /|        ___/|______halfEdgeLen
     *     / |    .__/__/ |
     *    /  |   / \   /  |
     *   /___|__/___\_/   |
     *   |   | /     \|   |
     *   |   |/       |___|
     *   |   /        |\  /
     *   |  /_________|_\/
     *   | /     /    | /
     *   |/_____/_____|/
     *          \___________halfEdgeLen
     */
    Octree(const Octree<T>& copy, int *counter, int viewportType = -1)
        : center(copy.center), halfEdgeLen(copy.halfEdgeLen) {
        positions = copy.positions;
        for(int i=0; i<8; ++i) {
            children[i] = NULL;
        }
        if(copy.hasContent()) {
            (*counter)++;
            objects = copy.objects;
        }
        if(copy.isLeaf()) {
            return;
        }
        if(viewportType != -1 and halfEdgeLen*2 < 1./state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx) {
            if(copy.hasContent() == false) {
                return;
            }
            Triangle tri;
            objects.push_back(tri);
            objects[0].a.x = center.x - halfEdgeLen;
            objects[0].a.y = center.y;
            objects[0].a.z = center.z - halfEdgeLen;
            objects[0].b.x = center.x + halfEdgeLen;
            objects[0].b.y = center.y;
            objects[0].b.z = center.z - halfEdgeLen;
            objects[0].c.x = center.x;
            objects[0].c.y = center.y;
            objects[0].c.z = center.z + halfEdgeLen;
            return;
        }
        for(int i = 0; i < 8; ++i) {
            if(copy.children[i] != NULL) {
                children[i] = new Octree(*(copy.children[i]), counter, viewportType);
                continue;
            }
            children[i] = NULL;
        }
    }

    ~Octree() {
        // Recursively destroy octants
        for(int i = 0; i < 8; ++i) {
            delete children[i];
            children[i] = NULL;
        }
    }

    /**
    * @brief Returns the octant in which this coordinate belongs.
    * @param pos the float coordinate whose octant is looked for.
    * This implementation uses following rules for the children:
    * Here, - means less than 'center' in that dimension, + means greater than.
    * child: 0 1 2 3 4 5 6 7
    * x:     - - - - + + + +
    * y:     - - + + - - + +
    * z:     - + - + - + - +
    *
    **/
    int getOctantFor(const floatCoordinate pos) {
        int oct = 0;
        if(pos.x >= center.x) oct |= 4;
        if(pos.y >= center.y) oct |= 2;
        if(pos.z >= center.z) oct |= 1;
        return oct;
    }

    bool isLeaf() const {
        // leaves have no children
        return children[0] == NULL && children[1] == NULL && children[2] == NULL && children[3] == NULL
            && children[4] == NULL && children[5] == NULL && children[6] == NULL && children[7] == NULL;
    }

    /**
     * @brief isObjNode
     * @return true if this node holds an object, false otherwise
     */
    bool isObjNode() const {
        return objects.size() > 0;
    }

    /**
     * @brief hasContent tells you if this or any sub-octree holds an object
     * @return true if any this or a sub-octree holds an object, false otherwise
     */
    bool hasContent() const {
        if(objects.size() > 0) {
            return true;
        }
        else if(isLeaf()) {
            return false;
        }
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            if(children[i]->hasContent()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief contains tells you if this octree's range includes given point.
     *        If the point lies on the octree's borders, it is considered as inside.
     */
    bool contains(floatCoordinate point) {
        return center.x - halfEdgeLen <= point.x and center.x + halfEdgeLen >= point.x
                and center.y - halfEdgeLen <= point.y and center.y + halfEdgeLen >= point.y
                and center.z - halfEdgeLen <= point.z and center.z + halfEdgeLen >= point.z;
    }

    /**
     * @brief childContainsBBox returns true if given child octant contains given bounding box
     * @param bboxMin min coordinate of bounding box
     * @param bboxMax max coordinate of bounding box
     * @param childOctant the child to check against
     * @return true if bounding box fits into child octant, false otherwise
     */
    bool childContainsBBox(floatCoordinate bboxMin, floatCoordinate bboxMax, int childOctant) {
        switch(childOctant) {
        case 0:
            if(bboxMin.x < center.x - halfEdgeLen or bboxMax.x > center.x
               or bboxMin.y < center.y - halfEdgeLen or bboxMax.y > center.y
               or bboxMin.z < center.z - halfEdgeLen or bboxMax.z > center.z) {
                return false;
            }
            break;
        case 1:
            if(bboxMin.x < center.x - halfEdgeLen or bboxMax.x > center.x
               or bboxMin.y < center.y - halfEdgeLen or bboxMax.y > center.y
               or bboxMin.z < center.z or bboxMax.z > center.z + halfEdgeLen) {
                return false;
            }
            break;
        case 2:
            if(bboxMin.x < center.x - halfEdgeLen or bboxMax.x > center.x
               or bboxMin.y < center.y or bboxMax.y > center.y + halfEdgeLen
               or bboxMin.z < center.z - halfEdgeLen or bboxMax.z > center.z) {
                return false;
            }
            break;
        case 3:
            if(bboxMin.x < center.x - halfEdgeLen or bboxMax.x > center.x
               or bboxMin.y < center.y or bboxMax.y > center.y + halfEdgeLen
               or bboxMin.z < center.z or bboxMax.z > center.z + halfEdgeLen) {
                return false;
            }
            break;
        case 4:
            if(bboxMin.x < center.x or bboxMax.x > center.x + halfEdgeLen
               or bboxMin.y < center.y - halfEdgeLen or bboxMax.y > center.y
               or bboxMin.z < center.z - halfEdgeLen or bboxMax.z > center.z) {
                return false;
            }
            break;
        case 5:
            if(bboxMin.x < center.x or bboxMax.x > center.x + halfEdgeLen
               or bboxMin.y < center.y - halfEdgeLen or bboxMax.y > center.y
               or bboxMin.z < center.z or bboxMax.z > center.z + halfEdgeLen) {
                return false;
            }
            break;
        case 6:
            if(bboxMin.x < center.x or bboxMax.x > center.x + halfEdgeLen
               or bboxMin.y < center.y or bboxMax.y > center.y + halfEdgeLen
               or bboxMin.z < center.z - halfEdgeLen or bboxMax.z > center.z) {
                return false;
            }
            break;
        case 7:
            if(bboxMin.x < center.x or bboxMax.x > center.x + halfEdgeLen
               or bboxMin.y < center.y or bboxMax.y > center.y + halfEdgeLen
               or bboxMin.z < center.z or bboxMax.z > center.z + halfEdgeLen) {
                return false;
            }
            break;
        default:
            return false;
        }
        return true;
    }

    /**
     * @brief insert inserts a new object into the octree.
     * @param newObject pointer to new object
     * @param pos the object's coordinates
     * @param allowList a flag to specify the function's behaviour on collision with an object at exactly the same position in space.
     *        add object to list (true) or replace old object (false).
     * @return returns true if a new object was added without replacing an existing one, false otherwise.
     */
//    bool insert(T newObject, floatCoordinate pos, bool replace, floatCoordinate bboxMin, floatCoordinate bboxMax,
//                std::vector<T> &removedObjs = Octree<T>::dummyVec) {
//        // If this node doesn't have an object yet assigned and it is a leaf, then we're done!
//        if(isLeaf()) {
//            if(hasContent() == false) {
//                objects.push_back(newObject);
//                point = pos;
//                return true;
//            }
//            else {
//                // We're at a leaf, but there's already something here.
//                // So make this leaf an interior node and store the old and new data in its leaves.
//                // Remember the object that was here for a later re-insert
//                // Do not reset this->point yet, as it is needed for re-insertion below

//                // first check if new object has same coordinate like old obj
//                if(point.x == pos.x && point.y == pos.y && point.z == pos.z) {
//                    if(replace) { // replacement asked, so delete old content
//                        if(&removedObjs != &dummyVec) {
//                            removedObjs.insert(removedObjs.end(), objects.begin(), objects.end());
//                        }
//                        objects.clear();
//                    }
//                    objects.push_back(newObject);
//                    return false;
//                }

//                std::vector<T> oldObjects = objects;

//                // Find child octants for old and new objects
//                int oct1 = getOctantFor(point);
//                int oct2 = getOctantFor(pos);
//                Octree<T> *o = this;
//                Octree<T> *otmp = NULL;
//                floatCoordinate newCenter;
//                while(oct1 == oct2) { // create new child until old and new object do not collide anymore
//                    if(bboxMin.x != -1) {
//                        if(childContainsBBox(bboxMin, bboxMax, oct1) == false) {
//                            o->objects.push_back(newObject);
//                            o->objects.insert(o->objects.back(), oldObjects.begin(), oldObjects.end());
//                            o->point = pos;
//                            return true;
//                        }
//                    }
//                    newCenter = o->getCenterOfChild(oct1);
//                    otmp = o;
//                    o = new Octree<T>(newCenter, otmp->halfEdgeLen*.5f);
//                    otmp->children[oct1] = o;
//                    oct1 = o->getOctantFor(point);
//                    oct2 = o->getOctantFor(pos);
//                }
//                // finally insert old and new objects to their new leaves
//                // if new object's bounding box would not fit into child octant, add it to this node
//                if(bboxMin.x != -1) {
//                    if(childContainsBBox(bboxMin, bboxMax, oct1) == false) {
//                        objects.push_back(newObject);
//                        point = pos;
//                        return true;
//                    }
//                }

//                newCenter = o->getCenterOfChild(oct1);
//                o->children[oct1] = new Octree<T>(newCenter, o->halfEdgeLen*.5f);
//                if(oldObjects.size() > 1) {
//                    for(uint i = 0; i < oldObjects.size(); ++i) {
//                        o->children[oct1]->insert(oldObjects[i], point, false, bboxMin, bboxMax);
//                    }
//                }
//                else {
//                     o->children[oct1]->insert(oldObjects[0], point, replace, bboxMin, bboxMax);
//                }
//                point.x = point.y = point.z = -1; // now that the old objects are re-inserted, we can reset point
//                newCenter = o->getCenterOfChild(oct2);
//                o->children[oct2] = new Octree<T>(newCenter, o->halfEdgeLen*.5f);
//                o->children[oct2]->insert(newObject, pos, replace, bboxMin, bboxMax, removedObjs);
//                return true;
//            }
//        } else {
//            // We are at an interior node. Insert recursively into the
//            // appropriate child octant

//            // first check if object is too big for a smaller octant
//            int octant = getOctantFor(pos);
//            if(children[octant] != NULL) {
//                return children[octant]->insert(newObject, pos, replace, bboxMin, bboxMax, removedObjs);
//            } //child does not exist yet
//            children[octant] = new Octree<T>(getCenterOfChild(octant), halfEdgeLen*.5f);
//            children[octant]->objects.push_back(newObject);
//            children[octant]->point = pos;
//            return true;
//        }
//    }

    bool insert(T newObject, floatCoordinate pos, bool replace, floatCoordinate bboxMin, floatCoordinate bboxMax,
                std::vector<T> &removedObjs = Octree<T>::dummyVec) {
        // If this node doesn't have an object yet assigned and it is a leaf, we're done!
        if(isLeaf()) {
            if(hasContent() == false) {
                objects.push_back(newObject);
                positions.push_back(pos);
                return true;
            }
            else {
                // We're at a leaf, but there's already something here.
                // So make this leaf an interior node and store the new data in one of its octants.

                // first, if object has no bounding box, check if new object has same coordinate like an old obj
                if(bboxMin.x == -1) {
                    for(uint i = 0; i < positions.size(); ++i) {
                        if(positions[i].x == pos.x && positions[i].y == pos.y && positions[i].z == pos.z) {
                            if(replace) {
                                if(&removedObjs != &dummyVec) {
                                    removedObjs.push_back(objects[i]);
                                }
                                objects.erase(objects.begin() + i);
                                objects.emplace(objects.begin() + i, newObject);
                            }
                            return false;
                        }
                    }
                }

                // Find child octant for new object
                int oct = getOctantFor(pos);

                // if new object's bounding box would not fit into child octant, add it to this node instead
                if(bboxMin.x != -1) {
                    if(childContainsBBox(bboxMin, bboxMax, oct) == false) {
                        objects.push_back(newObject);
                        positions.push_back(pos);
                        return true;
                    }
                }
                // object would fit into child octant, so create it
                Octree<T> *o = this;
                floatCoordinate newCenter;
                newCenter = o->getCenterOfChild(oct);
                o->children[oct] = new Octree<T>(newCenter, o->halfEdgeLen*.5f);
                o->children[oct]->insert(newObject, pos, replace, bboxMin, bboxMax);
                return true;
            }
        } else {
            // We are at an interior node. Insert recursively into the appropriate child octant

            // first check if object is too big for a smaller octant
            int octant = getOctantFor(pos);
            if(bboxMin.x != -1) {
                if(childContainsBBox(bboxMin, bboxMax, octant) == false) {
                    objects.push_back(newObject);
                    positions.push_back(pos);
                    return true;
                }
            }
            // object fits into child octant
            if(children[octant] != NULL) {
                return children[octant]->insert(newObject, pos, replace, bboxMin, bboxMax, removedObjs);
            } //child does not exist yet
            children[octant] = new Octree<T>(getCenterOfChild(octant), halfEdgeLen*.5f);
            children[octant]->insert(newObject, pos, replace, bboxMin, bboxMax);
            return true;
        }
    }

    /**
     * @brief getAllObjs writes all 'objects' found in this octree into the provided objs vector.
     * @param objs a vector<T> to hold all the 'objects' found in this octree.
     */
    void getAllObjs(std::vector<T> &objs) {
        if(isObjNode()) {
            objs.insert(objs.end(), objects.begin(), objects.end());
        }
        if(isLeaf()) {
            return;
        }
        // interior node
        for(int i = 0; i < 8; i++) {
            if(children[i] == NULL) {
                continue;
            }
            children[i]->getAllObjs(objs);
        }
    }

    /**
     * @brief getAllVisibleObjs retrieves all objects from the octree which are inside the viewing frustum of specified viewport.
     * @param objs a vector of objects to which this method writes recursively while going deeper into the octree.
     * @param viewportType VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ or VIEWPORT_SKELETON
     */
    void getAllVisibleObjs(std::vector<T> &objs, uint viewportType) {
        //check if in frustum
        if(Renderer::cubeInFrustum(center, halfEdgeLen*2, viewportType) == false) {
            return;
        }
        if(isObjNode()) {
            objs.insert(objs.end(), objects.begin(), objects.end());
        }
        if(isLeaf()) {
            return;
        }
        // interior node
        for(int i = 0; i < 8; i++) {
            if(children[i] == NULL) {
                continue;
            }
            children[i]->getAllVisibleObjs(objs, viewportType);
        }
    }

    /**
     * @brief getCenterOfChild returns the center of this octree's child i.
     * @param i must be in range [0, 7]
     * Children follow a predictable pattern to make accesses simple:
     *        ____________
     *       /|    /     /|
     *      /_|1__/|_5__/ |
     *     /| |  /||   /| |
     *    /_3_|_/_||7_/_|_|
     *    | |/  | |/  | | /
     *    | /___|_|___|_|/|
     *    |/    |/    | / |
     *    |_____|_____|/|_|
     *    | |/  | |/  | | /
     *    | /_0_|_|_4_|_|/
     *    |/    |/    | /
     *    |_2___|__6__|/
     **/
    floatCoordinate getCenterOfChild(const int i) {
        floatCoordinate c;
        switch(i) {
        case(0):
            c.x = center.x - .5f * halfEdgeLen;
            c.y = center.y - .5f * halfEdgeLen;
            c.z = center.z - .5f * halfEdgeLen;
            break;
        case(1):
            c.x = center.x - .5f * halfEdgeLen;
            c.y = center.y - .5f * halfEdgeLen;
            c.z = center.z + .5f * halfEdgeLen;
            break;
        case(2):
            c.x = center.x - .5f * halfEdgeLen;
            c.y = center.y + .5f * halfEdgeLen;
            c.z = center.z - .5f * halfEdgeLen;
            break;
        case(3):
            c.x = center.x - .5f * halfEdgeLen;
            c.y = center.y + .5f * halfEdgeLen;
            c.z = center.z + .5f * halfEdgeLen;
            break;
        case(4):
            c.x = center.x + .5f * halfEdgeLen;
            c.y = center.y - .5f * halfEdgeLen;
            c.z = center.z - .5f * halfEdgeLen;
            break;
        case(5):
            c.x = center.x + .5f * halfEdgeLen;
            c.y = center.y - .5f * halfEdgeLen;
            c.z = center.z + .5f * halfEdgeLen;
            break;
        case(6):
            c.x = center.x + .5f * halfEdgeLen;
            c.y = center.y + .5f * halfEdgeLen;
            c.z = center.z - .5f * halfEdgeLen;
            break;
        case(7):
            c.x = center.x + .5f * halfEdgeLen;
            c.y = center.y + .5f * halfEdgeLen;
            c.z = center.z + .5f * halfEdgeLen;
            break;
        default:
            c.x = c.y = c.z = -1;
        }
        return c;
    }

    /**
     * @brief getObjsOnLine get objects on arbitrary line pq. It doesn't matter if you pass p and q or q and p.
     * @param results found objects are written to results.
     */
    void getObjsOnLine(floatCoordinate p, floatCoordinate q, std::vector<T> &results) {
        if(p.x == q.x and p.y == q.y and p.z == q.z) {
            return;
        }
        floatCoordinate p_point, point_q, p_q;
        SUB_ASSIGN_COORDINATE(p_q, q, p);

        if(isObjNode()) {
            for(uint i = 0; i < positions.size(); ++i) {
                SUB_ASSIGN_COORDINATE(p_point, positions[i], p);
                SUB_ASSIGN_COORDINATE(point_q, q, positions[i]);
                if(almostEqual(euclidicNorm(&p_point) + euclidicNorm(&point_q), euclidicNorm(&p_q))) {
                    results.push_back(objects[i]);
                }
            }
        }
        if(isLeaf()) {
            return;
        }

        // interior node, check children
        int minX = (p_q.x > 0)? p.x : q.x; int maxX = (p_q.x > 0)? q.x : p.x;
        int minY = (p_q.y > 0)? p.y : q.y; int maxY = (p_q.y > 0)? q.y : p.y;
        int minZ = (p_q.z > 0)? p.z : q.z; int maxZ = (p_q.z > 0)? q.z : p.z;

        if(center.x + halfEdgeLen < minX or center.x - halfEdgeLen > maxX
                or center.y + halfEdgeLen < minY or center.y - halfEdgeLen > maxY
                or center.z + halfEdgeLen < minZ or center.z - halfEdgeLen > maxZ) {
            // line does not intersect this octant, don't check children
            return;
        }
        // line intersects this octant
        for(int i = 0; i < 8; ++i) {
            if(children[i] != NULL) {
                children[i]->getObjsOnLine(p, q, results);
            }
        }
    }

    /**
     * @brief getObjsOnLine get objects on orthogonal line.
     *        The line lies on a plane, has a start point and runs parallel to one of the axes, i.e.
     *        for line in xy plane pass z value (plane), y (start point) and -1 for x.
     *        Uses depth first search and order of results follows the order of child octants (see 'getCenterOFChild').
     * @param tolerance: points with distance to the line smaller than 'tolerance' are accepted as on the line
     * @param results found objects are written to results
     */
    void getObjsOnLine(std::vector<float> &results, int x, int y, int z) {
        if(x == -1 and y == -1 and z == -1) {
            return;
        }
        if(isObjNode()) {
            if(z == -1) {
                for(floatCoordinate point : positions) {
                    if(floor(point.x) == x and floor(point.y) == y) {
                        results.push_back(point.z);
                    }
                }
            }
            else if(y == -1) {
                for(floatCoordinate point : positions) {
                    if(floor(point.x) == x and floor(point.z) == z) {
                        results.push_back(point.y);
                    }
                }
            }
            else if(x == -1) {
                for(floatCoordinate point : positions) {
                    if(floor(point.y) == y and floor(point.z) == z) {
                        results.push_back(point.x);
                    }
                }
            }
        }
        if(isLeaf()) {
            return;
        }
        // interior node, check children
        if(z == -1) {
            if((center.x + halfEdgeLen < x and center.x - halfEdgeLen > x)
                    or (center.y + halfEdgeLen < y and center.y - halfEdgeLen > y)) {
                return;
            }
        }
        if(y == -1) {
            if((center.x + halfEdgeLen < x and center.x - halfEdgeLen > x)
                    or (center.z + halfEdgeLen < z and center.z - halfEdgeLen > z)) {
                return;
            }
        }
        if(x == -1) {
            if((center.y + halfEdgeLen < y and center.y - halfEdgeLen > y)
                    or (center.z + halfEdgeLen < z and center.z - halfEdgeLen > z)) {
                return;
            }
        }
        // line intersects octant. check children
        for(int i = 0; i < 8; ++i) {
            if(children[i] != NULL) {
                children[i]->getObjsOnLine(results, x, y, z);
            }
        }
    }

    /**
     * @brief getNearestNeighbor returns pointer to the nearest neighbor of the object at given position.
     *        If many nearest neighbors exist, the first one found is returned.
     *        Returns NULL if no neighbor is found
     */
    std::pair<floatCoordinate, T> getNearestNeighbor(floatCoordinate pos) {
        // dummy return for no nearest neighbor
        T dummy;
        floatCoordinate dummyPos;
        dummyPos.x = -1;

        if(isObjNode()) {
            float minDist = INT_MAX;
            int neighbor;
            for(uint i = 0; i < positions.size(); ++i) {
                if(minDist > distance(pos, positions[i])) {
                    minDist = distance(pos, positions[i]);
                    neighbor = i;
                }
            }
            return std::pair<floatCoordinate, T>(positions[neighbor], objects[neighbor]);
        }
        else if(isLeaf()) {
            return std::pair<floatCoordinate, T>(dummyPos, dummy);
        }

        // interior node
        if(hasContent() == false) {
            return std::pair<floatCoordinate, T>(dummyPos, dummy);
        }

        // rearrange children for faster search
        Octree<T> *searchOrder[8];
        for(int i = 0; i < 8; ++i) { // order of search if point is inside the octant or left of it.
            searchOrder[i] = children[i];
        }
        if(contains(pos) == false) {
            if(center.x + halfEdgeLen < pos.x) { // point right of octant. so explore children 4 - 7 before the others
                for(int i = 0; i < 4; ++i) {
                    searchOrder[i] = children[i + 4];
                    searchOrder[i + 4] = children[i];
                }
            }
            else if(center.y - halfEdgeLen > pos.y) { // point behind octant. explore 0, 1, 4, 5 before the others
                searchOrder[0] = children[0]; searchOrder[1] = children[1];
                searchOrder[2] = children[4]; searchOrder[3] = children[5];
                searchOrder[4] = children[2]; searchOrder[5] = children[3];
                searchOrder[6] = children[6]; searchOrder[7] = children[7];
            }
            else if(center.y + halfEdgeLen < pos.y) { // point in front of octant. explore 2, 3, 6, 7 before the others
                searchOrder[0] = children[2]; searchOrder[1] = children[3];
                searchOrder[2] = children[6]; searchOrder[3] = children[7];
                searchOrder[4] = children[0]; searchOrder[5] = children[1];
                searchOrder[6] = children[4]; searchOrder[7] = children[5];

            }
            else if(center.z - halfEdgeLen > pos.z) { // point below octant. explore 0, 2, 4, 6 before the others
                searchOrder[0] = children[0]; searchOrder[1] = children[2];
                searchOrder[2] = children[4]; searchOrder[3] = children[6];
                searchOrder[4] = children[1]; searchOrder[5] = children[3];
                searchOrder[6] = children[5]; searchOrder[7] = children[7];
            }
            else if(center.z + halfEdgeLen < pos.z) { // point over octant. explore 1, 3, 5, 7 before the others
                searchOrder[0] = children[0]; searchOrder[1] = children[2];
                searchOrder[2] = children[4]; searchOrder[3] = children[6];
                searchOrder[4] = children[1]; searchOrder[5] = children[3];
                searchOrder[6] = children[5]; searchOrder[7] = children[7];
            }
        }
        std::vector<std::pair<floatCoordinate, T> > neighbors;
        for(int i = 0; i < 4; ++i) {
            if(searchOrder[i] == NULL) {
                continue;
            }
            std::pair<floatCoordinate, T> found = searchOrder[i]->getNearestNeighbor(pos);
            if(found.first.x != -1) {
                neighbors.push_back(found);
            }
        }
        if(neighbors.size() == 0) { // in first four sub octants no objects. Check remaining 4.
            for(int i = 4; i < 8; ++i) {
                if(searchOrder[i] == NULL) {
                    continue;
                }
                std::pair<floatCoordinate, T> found = searchOrder[i]->getNearestNeighbor(pos);
                if(found.first.x != -1) {
                    neighbors.push_back(found);
                }
            }
        }
        std::pair<floatCoordinate, T> neighbor;
        float tmpDist;
        float minDist = INT_MAX;
        for(uint i = 0; i < neighbors.size(); ++i) {
            if((tmpDist = distance(pos, neighbors[i].first)) < minDist) {
                minDist = tmpDist;
                neighbor = neighbors[i];
            }
        }
        return neighbor;
    }

    /**
     * @brief getObjsInRange retrieves all objects found in the given bounding box  and stores them in results.
     * @param pos center of the bounding box
     * @param halfCubeLen half the edge length of the bounding box
     * @param results a vector<T> to store all found objects in.
     */
    void getObjsInRange(const floatCoordinate pos, float halfCubeLen, std::vector<T> &results) {
        floatCoordinate bmin, bmax;
        bmin.x = pos.x - halfCubeLen; bmin.y = pos.y - halfCubeLen; bmin.z = pos.z - halfCubeLen;
        bmax.x = pos.x + halfCubeLen; bmax.y = pos.y + halfCubeLen; bmax.z = pos.z + halfCubeLen;

        // First see if octant intersects bounding box
        if(center.x + halfEdgeLen < bmin.x || center.x - halfEdgeLen > bmax.x
           || center.y + halfEdgeLen < bmin.y || center.y - halfEdgeLen > bmax.y
           || center.z + halfEdgeLen < bmin.z || center.z - halfEdgeLen > bmax.z ) {
            return;
        }

        // octant intersects bounding box
        // check this node's objects
        if(isObjNode()) {
            for(uint i = 0; i < positions.size(); ++i) {
                if(positions[i].x > bmax.x || positions[i].y > bmax.y || positions[i].z > bmax.z) continue;
                if(positions[i].x < bmin.x || positions[i].y < bmin.y || positions[i].z < bmin.z) continue;
                results.push_back(objects[i]);
            }
        }
        if(isLeaf()) {
            return;
        }

        // We're at an interior node of the tree.
        // check children
        for(int i=0; i<8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            // Compute the min/max corners of this child octant
            floatCoordinate cmax;
            floatCoordinate cmin;
            cmax.x = children[i]->center.x + children[i]->halfEdgeLen;
            cmax.y = children[i]->center.y + children[i]->halfEdgeLen;
            cmax.z = children[i]->center.z + children[i]->halfEdgeLen;
            cmin.x = children[i]->center.x - children[i]->halfEdgeLen;
            cmin.y = children[i]->center.y - children[i]->halfEdgeLen;
            cmin.z = children[i]->center.z - children[i]->halfEdgeLen;

            // If the query rectangle is outside the child's bounding box,
            // then continue
            if(cmax.x < bmin.x || cmax.y < bmin.y || cmax.z < bmin.z) continue;
            if(cmin.x > bmax.x || cmin.y > bmax.y || cmin.z > bmax.z) continue;

            // if child completely in bounding box, simply add all objects of this octree to the result
            if(cmax.x < bmax.x && cmin.x > bmin.x
               && cmax.y < bmax.y && cmin.y > bmin.y
               && cmax.z < bmax.z && cmin.z > bmin.z) {
                getAllObjs(results);
                continue;
            }
            // this child only intersects the query bounding box
            children[i]->getObjsInRange(pos, halfCubeLen, results);
        }
    }

    /**
     * @brief getObjsInMargin
     * @param pos
     * @param halfCubeLen
     * @param margin
     * @param results
     *     ______________
     *    /   margin    /|
     *   /  ___________/ |
     *  /__////////////| |
     *  | ////////////|| |
     *  | |/////////|/|| |
     *  | |///cube//|/|/ |
     *  | |/////////|/|  /
     *  | |/////////|/| /
     *  |_____________|/
     */
    void getObjsInMargin(floatCoordinate pos, uint halfCubeLen, uint margin, std::vector<T> results) {
        if(isObjNode()) {
            for(uint i = 0; i < positions.size(); ++i) {
                // check if object outside of cube + margin
                if(positions[i].x > pos.x + halfCubeLen + margin
                   or positions[i].y > pos.y + halfCubeLen + margin
                   or positions[i].z > pos.z + halfCubeLen + margin) {
                    continue;
                }
                if(positions[i].x < pos.x - halfCubeLen - margin
                   or positions[i].y < pos.y - halfCubeLen - margin
                   or positions[i].z < pos.z - halfCubeLen - margin) {
                    continue;
                }
                // check if object in cube
                if(positions[i].x < pos.x + halfCubeLen and positions[i].x > pos.x - halfCubeLen
                   and positions[i].y < pos.y + halfCubeLen and positions[i].y > pos.y - halfCubeLen
                   and positions[i].z < pos.z + halfCubeLen and positions[i].z > pos.z - halfCubeLen) {
                    continue;
                }
                // object in margin area
                results.push_back(objects[i]);
            }
        }
        if(isLeaf()) {
            return;
        }

        // check if octree intersects margin
        floatCoordinate cmin, cmax;
        cmin.x = center.x - halfEdgeLen;
        cmin.y = center.y - halfEdgeLen;
        cmin.z = center.z - halfEdgeLen;
        cmax.x = center.x + halfEdgeLen;
        cmax.y = center.y + halfEdgeLen;
        cmax.z = center.z + halfEdgeLen;
        // check if octree outside of cube + margin
        if(cmin.x > pos.x + halfCubeLen + margin
           or cmin.y > pos.y + halfCubeLen + margin
           or cmin.z > pos.z + halfCubeLen + margin) {
            return;
        }
        if(cmax.x < pos.x - halfCubeLen - margin
           or cmax.y < pos.y - halfCubeLen - margin
           or cmax.z < pos.z - halfCubeLen - margin) {
            return;
        }
        // check if octree within cube
        if(cmax.x < pos.x + halfCubeLen and cmin.x > pos.x - halfCubeLen
           and cmax.y < pos.y + halfCubeLen and cmin.y > pos.y - halfCubeLen
           and cmax.z < pos.z + halfCubeLen and cmin.z > pos.z - halfCubeLen) {
            return;
        }
        // octree intersects margin
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            cmin.x = children[i]->center.x - children[i]->halfEdgeLen;
            cmin.y = children[i]->center.y - children[i]->halfEdgeLen;
            cmin.z = children[i]->center.z - children[i]->halfEdgeLen;
            cmax.x = children[i]->center.x + children[i]->halfEdgeLen;
            cmax.y = children[i]->center.y + children[i]->halfEdgeLen;
            cmax.z = children[i]->center.z + children[i]->halfEdgeLen;
            // check if child outside of cube + margin
            if(cmin.x > pos.x + halfCubeLen + margin
               or cmin.y > pos.y + halfCubeLen + margin
               or cmin.z > pos.z + halfCubeLen + margin) {
                continue;
            }
            if(cmax.x < pos.x - halfCubeLen - margin
               or cmax.y < pos.y - halfCubeLen - margin
               or cmax.z < pos.z - halfCubeLen - margin) {
                continue;
            }
            // check if child within cube
            if(cmax.x < pos.x + halfCubeLen and cmin.x > pos.x - halfCubeLen
               and cmax.y < pos.y + halfCubeLen and cmin.y > pos.y - halfCubeLen
               and cmax.z < pos.z + halfCubeLen and cmin.z > pos.z - halfCubeLen) {
                continue;
            }
            // child intersects margin
            children[i]->getObjsInMargin(pos, halfCubeLen, margin, results);
        }
    }

    /**
     * @brief remove removes given object from octree. The "==" operator must be defined for this to work.
     * @param obj the object to be removed
     * @param objPos the object's position
     * @return true if something was removed, false otherwise
     */
    bool remove(T obj, floatCoordinate objPos) {
        if(contains(objPos) == false) {
            return false;
        }
        if(isObjNode()) {
            for(uint i = 0; i < objects.size(); ++i) {
                if(objects[i] == obj) {
                    objects.erase(objects.begin() + i);
                    positions.erase(positions.begin() + i);
                    return true;
                }
            }
        }
        if(isLeaf()) {
            return false;
        }

        if(center.x + halfEdgeLen < objPos.x or center.x - halfEdgeLen > objPos.x
           or center.y + halfEdgeLen < objPos.y or center.y - halfEdgeLen > objPos.y
           or center.z + halfEdgeLen < objPos.z or center.z - halfEdgeLen > objPos.z) {
            return false;
        }
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            if(children[i]->isLeaf() == false) { // not at level above leaf, yet
                if(children[i]->remove(obj, objPos)) {
                    if(children[i]->hasContent() == false) {
                        delete children[i];
                        children[i] = NULL;
                    }
                    return true;
                }
            }
            else if(children[i]->hasContent() == false) { // leaf is empty
                continue;
            }
            else { // found leaf that is not empty
                for(uint j = 0; j < children[i]->objects.size(); ++j) {
                    if(obj == children[i]->objects[j]) {
                        children[i]->objects.erase(children[i]->objects.begin() + j);
                        children[i]->positions.erase(children[i]->positions.begin() + j);
                        if(children[i]->objects.size() == 0) {
                            delete children[i];
                            children[i] = NULL;
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * @brief clearObjsInBBox deletes all objects in specified bounding box
     * @param pos the center of the bounding box
     * @param halfCubeLen half the edge length of the bounding box
     */
    void clearObjsInBBox(const floatCoordinate pos, uint halfCubeLen, std::vector<T> &removedObjs = Octree<T>::dummyVec) {
        // check if given bounding box completely contains this octree
        if(center.x - halfEdgeLen > pos.x - halfCubeLen and center.x + halfEdgeLen < pos.x + halfCubeLen
           and center.y - halfEdgeLen > pos.y - halfCubeLen and center.y + halfEdgeLen < pos.y + halfCubeLen
           and center.z - halfEdgeLen > pos.z - halfCubeLen and center.z + halfEdgeLen < pos.z + halfCubeLen) {
            if(&removedObjs != &dummyVec) {
                getAllObjs(removedObjs);
            }
            for(int i = 0; i < 8; ++i) {
                delete children[i];
            }
            return;
        }
        // bounding box does not completely contain the octree

        if(isObjNode()) {
            for(uint i = 0; i < positions.size(); ++i) {
                if(positions[i].x > pos.x - halfCubeLen and positions[i].x < pos.x + halfCubeLen
                   and positions[i].y > pos.y - halfCubeLen and positions[i].y < pos.y + halfCubeLen
                   and positions[i].z > pos.z - halfCubeLen and positions[i].z < pos.z + halfCubeLen) {
                    if(&removedObjs != &dummyVec) {
                        removedObjs.push_back(objects[i]);
                    }
                    objects.erase(objects.begin() + i);
                    positions.erase(positions.begin() + i);
                }
            }
        }
        if(isLeaf()) {
            return;
        }

        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            children[i]->clearObjsInBBox(pos, halfCubeLen, removedObjs);
            if(children[i]->hasContent() == false) {
                delete children[i];
                children[i] = NULL;
            }
        }
    }

    /**
     * @brief objInRange Checks if there is an object in specified box around the given position 'pos'.
     * @param pos center of search cube.
     * @param range the size of the search cube.
     * @return true if there is an object in range, false otherwise.
     */
    bool objInRange(floatCoordinate pos, float range) {
        if(isObjNode()) {
            for(uint i = 0; i < positions.size(); ++i) {
                if(positions[i].x <= (pos.x + range) and positions[i].x >= (pos.x - range)
                   and positions[i].y <= (pos.y + range) and positions[i].y >= (pos.y - range)
                   and positions[i].z <= (pos.z + range) and positions[i].z >= (pos.z - range)) {
                    return true;
                }
            }
        }
        if(isLeaf()) {
            return false;
        }
        // interior node. check if octree intersects with range
        if(center.x + halfEdgeLen < pos.x - range or center.x - halfEdgeLen > pos.x + range
           or center.y + halfEdgeLen < pos.y - range or center.y - halfEdgeLen > pos.y + range
           or center.z + halfEdgeLen < pos.z - range or center.z - halfEdgeLen > pos.z + range) {
            return false; //octree is not in range
        }
        //octree intersects with range
        for(int i = 0; i < 8; i++) {
            if(children[i] == NULL) {
                continue;
            }
            if(children[i]->objInRange(pos, range)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief writeTo outputs octree to stream.
     * @param out the stream to write to
     * @param index Don't specify this! Internal helper string that holds the current octree's index. Root has index 0.
     *              Octree with index 0.2.4.5 is the fifth child of the fourth child of the second child of root.
     * Recursively outputs the octree to Stream. Starts at root and prints out all its children, then calls writeTo on all of its children.
     * Format:
     *  0: center(1024, 896, 1024)
     *  children:
     *  0.0: center(512, 384, 512)
     *  0.1: center(512, 384, 1536)
     *  0.2: center(512, 1408, 512)
     *  0.3: center(512, 1408, 1536), pos(50, 124, 144), pos(54, 122, 144)
     *  0.4: center(1536, 384, 512)
     *  0.5: center(1536, 384, 1536)
     *  0.6: center(1536, 1408, 512), pos(86, 27, 287)
     *  0.7: center(1536, 1408, 1536)
     *
     *  Next level:
     *  0.0: center(512, 384, 512)
     *  children:
     *  0.0.0: center(256, 128, 256)
     *  0.0.1: center(256, 128, 768)
     *  0.0.2: center(256, 640, 256)
     *  ...
     *
     **/
    void writeTo(QTextStream &out, QString index) {
        if(isObjNode()) {
            out << index
                << ": center(" << center.x << ", " << center.y << ", " << center.z
                << ")";
            for(floatCoordinate pos : positions) {
            out << ", pos(" << pos.x << ", " << pos.y << ", " << pos.z << ")";
            }
            out << "\n";
        }
        if(isLeaf()) {
            return;
        }

        out << index << ": center(" << center.x << ", " << center.y << ", " << center.z << ")\n";
        out << "children:\n";
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            if(isObjNode() == false) {
                out << index << "." << i << ": center("
                    << children[i]->center.x
                    << ", " << children[i]->center.y
                    << ", " << children[i]->center.z << ")\n";
            }
            else {
                out << index << "." << i << ": center("
                    << children[i]->center.x
                    << ", "<< children[i]->center.y
                    << ", " << children[i]->center.z
                    << ")";
                for(uint j = 0; j < children[i]->positions.size(); ++j) {
                    out << ", pos("
                        << children[i]->positions[j].x
                        << ", " << children[i]->positions[j].y
                        << ", " << children[i]->positions[j].z << ")";
                }
                out << "\n";
            }
        }
        out << "\nNext level:\n";
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            children[i]->writeTo(out, QString("%1.%2").arg(index).arg(i));
        }
    }
};

template<typename T> std::vector<T> Octree<T>::dummyVec;

#endif // OCTREE_H
