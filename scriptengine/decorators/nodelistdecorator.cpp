#include "nodelistdecorator.h"
#include "knossos-global.h"

NodeListDecorator::NodeListDecorator(QObject *parent) :
    QObject(parent)
{
}
/*
nodeListElement *NodeListDecorator::new_nodeListElement() {
    return new nodeListElement();
}

nodeListElement *NodeListDecorator::new_nodeListElement(int nodeID, int x, int y, int z, int parentID, float radius, int inVp, int inMag, int time) {
    return new nodeListElement(nodeID, x, y, z, parentID, radius, inVp, inMag, time);
}
*/

int NodeListDecorator::node_id(nodeListElement *self) {
    return self->nodeID;
}

bool NodeListDecorator::is_branch_node(nodeListElement *self) {
    return self->isBranchNode;
}

QList<segmentListElement *> *NodeListDecorator::segments(nodeListElement *self) {
    return self->getSegments();
}

char *NodeListDecorator::comment(nodeListElement *self) {
    if(self->comment) {
        return self->comment->content;
    }

    return 0;
}

int NodeListDecorator::time(nodeListElement *self) {
    return self->timestamp;
}

float NodeListDecorator::radius(nodeListElement *self) {
    return self->radius;
}

treeListElement *NodeListDecorator::parent_tree(nodeListElement *self) {
    return self->correspondingTree;
}

Coordinate NodeListDecorator::coordinate(nodeListElement *self) {
    return self->position;
}

int NodeListDecorator::mag(nodeListElement *self) {
    return self->createdInMag;
}

int NodeListDecorator::viewport(nodeListElement *self) {
    return self->createdInVp;
}

QString NodeListDecorator::static_nodeListElement_help() {
    return QString("the node class of knossos. You gain access to the following methods:" \
            "\n node_id() : returns the id of the current node" \
            "\n radius() : returns the radius of the current node" \
            "\n parent_tree() : returns the parent tree object of the current node" \
            "\n coordinate() : returns the coordinate object of the current node " \
            "\n comment() : returns the comment of the current node" \
            "\n is_branch_node() : returns a bool value indicating if the node is a branch node or not\n" \
            "\n viewport() : returns the viewport in which the node was created\n" \
            "\n mag() : returns the magnification in which the node was created\n" \
            "\n time() : returns the timestamp when the node was created");
}

/*
void NodeListDecorator::set_node_id(nodeListElement *self, int node_id) {
    self->nodeID = node_id;
}

void NodeListDecorator::set_comment(nodeListElement *self, char *comment) {
    if(!self->comment) {
        self->comment = new commentListElement();
        self->comment->content = comment;
        self->comment->node = self;
    }
}

void NodeListDecorator::set_time(nodeListElement *self, int time) {
    self->timestamp = time;
}

void NodeListDecorator::set_radius(nodeListElement *self, float radius) {
    self->radius = radius;
}

void NodeListDecorator::set_coordinate(nodeListElement *self, int x, int y, int z) {
    self->position.x = x;
    self->position.y = y;
    self->position.z = z;
}

void NodeListDecorator::set_coordinate(nodeListElement *self, Coordinate coordinate) {
    self->position = coordinate;
}

void NodeListDecorator::set_viewport(nodeListElement *self, int viewport) {
    self->createdInVp = viewport;
}

void NodeListDecorator::set_mag(nodeListElement *self, int magnification) {
    self->createdInMag = magnification;
}

void NodeListDecorator::set_parent_tree(nodeListElement *self, treeListElement *parent_tree) {
    self->correspondingTree = parent_tree;
}

void NodeListDecorator::set_parent_tree_id(nodeListElement *self, int id) {
    if(self->correspondingTree) {

    }
}
*/
