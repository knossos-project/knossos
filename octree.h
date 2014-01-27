#ifndef OCTREE_H
#define OCTREE_H

#include "knossos-global.h"
#include "renderer.h"

/**
 * The Octree template class.
 * Each node consumes 64 byte in a 32-bit application and 100 byte in a 64-bit.
 *
 */

extern struct stateInfo *state;

template<typename T> class Octree {
public:
    floatCoordinate center; //! The physical center of this node
    float halfEdgeLen;  //! Half the width/height/depth of this node

    Octree *children[8]; //! Pointers to child octants
    T *object;   //! the stored object, exists only in leaf nodes
    floatCoordinate point; //! coordinate of the object, (-1, -1, -1) if there is no object

    Octree(const floatCoordinate center, const float halfEdgeLen)
        : center(center), halfEdgeLen(halfEdgeLen), object(NULL) {
        // Initially, there are no children
        for(int i=0; i<8; ++i) {
            children[i] = NULL;
        }
        point.x = point.y = point.z = -1;
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
        : center(copy.center), halfEdgeLen(copy.halfEdgeLen), object(NULL) {
        point.x = copy.point.x;
        point.y = copy.point.y;
        point.z = copy.point.z;
        for(int i=0; i<8; ++i) {
            children[i] = NULL;
        }
        if(copy.isLeaf()) {
            if(copy.object != NULL) {
                (*counter)++;
                object = new T(*(copy.object));
            }
            return;
        }
        if(viewportType != -1 and halfEdgeLen*2 < 1./state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx) {
            if(copy.hasContent() == false) {
                return;
            }
            object = (Triangle *)malloc(sizeof(Triangle));
            object->a.x = center.x - halfEdgeLen;
            object->a.y = center.y;
            object->a.z = center.z - halfEdgeLen;
            object->b.x = center.x + halfEdgeLen;
            object->b.y = center.y;
            object->b.z = center.z - halfEdgeLen;
            object->c.x = center.x;
            object->c.y = center.y;
            object->c.z = center.z + halfEdgeLen;
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
        delete object;
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


    bool hasContent() const {
        if(isLeaf()) {
            return object != NULL;
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
     * @brief insert inserts a new object into the octree.
     * @param newObject pointer to new object
     * @param pos the objects coordinates in space
     * @param replace a flag to specify the functions behaviour on collision with an object at exactly the same position in space.
     *        Replace the object (true) or skip insertion (false).
     * @return returns true if a new triangle was added without replacing an existing one, false otherwise.
     */
    bool insert(T* newObject, floatCoordinate pos, bool replace) {
        // If this node doesn't have a data point yet assigned
        // and it is a leaf, then we're done!
        if(isLeaf()) {
            if(object == NULL) {
                object = newObject;
                point = pos;
                return true;
            }
            else {
                // We're at a leaf, but there's already something here.
                // So make this leaf an interior node and store the old and new data in its leaves.
                // Save the object that was here for a later re-insert
                // do not reset this->point yet, as it is needed for re-insertion below

                // first check if new object has same coordinate like old obj
                if(point.x == pos.x && point.y == pos.y && point.z == pos.z) {
                    if(replace) {
                        delete object;
                        object = newObject;
                        point = pos;
                        return false;
                    }
                    return false;
                }

                T *oldObject = object;
                object = NULL;
                // Find child octants for old and new object
                int oct1 = getOctantFor(point);
                int oct2 = getOctantFor(pos);
                Octree<T> *o = this;
                Octree<T> *otmp = NULL;
                floatCoordinate newCenter;
                while(oct1 == oct2) { // create new child until old and new object do not collide anymore
                    newCenter = o->getCenterOfChild(oct1);
                    otmp = o;
                    o = new Octree<T>(newCenter, otmp->halfEdgeLen*.5f);
                    otmp->children[oct1] = o;
                    oct1 = o->getOctantFor(point);
                    oct2 = o->getOctantFor(pos);
                }
                // finally insert old and new object to their new leaves
                newCenter = o->getCenterOfChild(oct1);
                o->children[oct1] = new Octree<T>(newCenter, o->halfEdgeLen*.5f);
                o->children[oct1]->insert(oldObject, point, replace);
                point.x = point.y = point.z = -1; // now that the old object is re-inserted, we can reset point
                newCenter = o->getCenterOfChild(oct2);
                o->children[oct2] = new Octree<T>(newCenter, o->halfEdgeLen*.5f);
                o->children[oct2]->insert(newObject, pos, replace);
                return true;
            }
        } else {
            // We are at an interior node. Insert recursively into the
            // appropriate child octant
            int octant = getOctantFor(pos);
            if(children[octant] != NULL) {
                return children[octant]->insert(newObject, pos, replace);
            } //child does not exist yet
            children[octant] = new Octree<T>(getCenterOfChild(octant), halfEdgeLen*.5f);
            children[octant]->object = newObject;
            children[octant]->point = pos;
            return true;
        }
    }

    /**
     * @brief getAllObjs writes all 'objects' found in this octree into the provided objs vector.
     * @param objs a vector<T*> to hold all the 'objects' found in this octree.
     */
    void getAllObjs(std::vector<T> &objs) {
        if(isLeaf()) {
            if(object != NULL) {
                objs.push_back(*object);
            }
            return;
        }
        // branch node
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
        if(isLeaf()) {
            if(object != NULL) {
                objs.push_back(*object);
            }
            return;
        }
        //check if in frustum
        if(Renderer::cubeInFrustum(center, halfEdgeLen*2, viewportType) == false) {
            return;
        }
        // branch node
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
     * @brief getObjsInRange retrieves all objects found in the given bounding box  and stores them in results.
     * @param pos center of the bounding box
     * @param halfCubeLen half the edge length of the bounding box
     * @param results a vector<T> to store all found objects in.
     */
    void getObjsInRange(const floatCoordinate pos, uint halfCubeLen, std::vector<T> &results) {
        // If we're at a leaf node, just see if the current data point is inside
        // the query bounding box
        floatCoordinate bmin, bmax;
        bmin.x = pos.x - halfCubeLen; bmin.y = pos.y - halfCubeLen; bmin.z = pos.z - halfCubeLen;
        bmax.x = pos.x + halfCubeLen; bmax.y = pos.y + halfCubeLen; bmax.z = pos.z + halfCubeLen;
        if(isLeaf()) {
            if(object!=NULL) {
                if(point.x > bmax.x || point.y > bmax.y || point.z > bmax.z) return;
                if(point.x < bmin.x || point.y < bmin.y || point.z < bmin.z) return;
                results.push_back(*object);
            }
            return;
        }
        // We're at an interior node of the tree.
        // First see if octant intersects bounding box
        if(center.x + halfEdgeLen < bmin.x || center.x - halfEdgeLen > bmax.x
           || center.y + halfEdgeLen < bmin.y || center.y - halfEdgeLen > bmax.y
           || center.z + halfEdgeLen < bmin.z || center.z - halfEdgeLen > bmax.z ) {
            return;
        }
        // octant intersects with bounding box.
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
            if(cmax.x<bmin.x || cmax.y<bmin.y || cmax.z<bmin.z) continue;
            if(cmin.x>bmax.x || cmin.y>bmax.y || cmin.z>bmax.z) continue;

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
        if(isLeaf()) {
            if(object == NULL) {
                return;
            }
            // check if object outside of cube + margin
            if(point.x > pos.x + halfCubeLen + margin
               or point.y > pos.y + halfCubeLen + margin
               or point.z > pos.z + halfCubeLen + margin) {
                return;
            }
            if(point.x < pos.x - halfCubeLen - margin
               or point.y < pos.y - halfCubeLen - margin
               or point.z < pos.z - halfCubeLen - margin) {
                return;
            }
            // check if object in cube
            if(point.x < pos.x + halfCubeLen and point.x > pos.x - halfCubeLen
               and point.y < pos.y + halfCubeLen and point.y > pos.y - halfCubeLen
               and point.z < pos.z + halfCubeLen and point.z > pos.z - halfCubeLen) {
                return;
            }
            // object in margin area
            results.push_back(*object);
        } // end isleaf
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
     * @brief clearObjsInCube deletes all leaves in this octree whose objects are in the specified cube.
     *        If this octree is a leaf itself, nothing happens.
     * @param pos the center of the specified cube
     * @param halfCubeLen half the edge length of specified cube
     */
    void clearObjsInCube(const floatCoordinate pos, uint halfCubeLen) {
        if(isLeaf()) {
            // self-deletion of leaves breaks the octree, because the parent pointer to this child cannot be set to NULL
            return;
        }
        // traverse the octree to one level above a leaf and delete the leaf if its object is inside the cube
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            if(children[i]->isLeaf() == false) { // not at level above leaf yet
                children[i]->clearObjsInCube(pos, halfCubeLen);
            }
            else if(object == NULL) { // leaf is empty
                continue;
            }
            else if(children[i]->point.x > pos.x - halfCubeLen and children[i]->point.x < pos.x + halfCubeLen
               and children[i]->point.y > pos.y - halfCubeLen and children[i]->point.y < pos.y + halfCubeLen
               and children[i]->point.z > pos.z - halfCubeLen and children[i]->point.z < pos.z + halfCubeLen) {
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
        if(isLeaf()) {
            if(object == NULL) {
                return false;
            }
            return point.x <= (pos.x + range) && point.x >= (pos.x - range)
                   && point.y <= (pos.y + range) && point.y >= (pos.y - range)
                   && point.z <= (pos.z + range) && point.z >= (pos.z - range);
        }
        // interior node. check if octree intersects with range
        if(center.x+halfEdgeLen < pos.x-range || center.x-halfEdgeLen > pos.x+range
           || center.y+halfEdgeLen < pos.y-range || center.y-halfEdgeLen > pos.y+range
           || center.z+halfEdgeLen < pos.z-range || center.z-halfEdgeLen > pos.z+range) {
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
     *  0.0: center(512, 384, 512) // internal node
     *  0.1: center(512, 384, 1536)
     *  0.2: center(512, 1408, 512)
     *  0.3: center(512, 1408, 1536), point(50, 124, 144) //leaf
     *  0.4: center(1536, 384, 512)
     *  0.5: center(1536, 384, 1536)
     *  0.6: center(1536, 1408, 512), point(86, 27, 287)
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
        if(isLeaf()) {
            out << index
                << ": center(" << center.x << ", " << center.y << ", " << center.z
                << "), point(" << point.x << ", " << point.y << ", " << point.z << ")\n";
            return;
        }
        out << index << ": center(" << center.x << ", " << center.y << ", " << center.z << ")\n";
        out << "children:\n";
        for(int i = 0; i < 8; ++i) {
            if(children[i] == NULL) {
                continue;
            }
            if(isLeaf() == false) {
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
                    << "), point("
                    << children[i]->point.x
                    << ", " << children[i]->point.y
                    << ", " << children[i]->point.z << ")\n";
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

#endif // OCTREE_H
