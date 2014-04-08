#include "nodelistdecorator.h"
#include "knossos-global.h"

NodeListDecorator::NodeListDecorator(QObject *parent) :
    QObject(parent)
{
}
/*
NodeListElement *NodeListDecorator::new_NodeListElement() {
    return new NodeListElement();
}

NodeListElement *NodeListDecorator::new_NodeListElement(int nodeID, int x, int y, int z, int parentID, float radius, int inVp, int inMag, int time) {
    return new NodeListElement(nodeID, x, y, z, parentID, radius, inVp, inMag, time);
}
*/

int NodeListDecorator::node_id(NodeListElement *self) {
    return self->nodeID;
}

bool NodeListDecorator::is_branch_node(NodeListElement *self) {
    return self->isBranchNode;
}

QList<SegmentListElement *> *NodeListDecorator::segments(NodeListElement *self) {
    return self->getSegments();
}

char *NodeListDecorator::comment(NodeListElement *self) {
    if(self->comment) {
        return self->comment->content;
    }

    return 0;
}

int NodeListDecorator::time(NodeListElement *self) {
    return self->timestamp;
}

float NodeListDecorator::radius(NodeListElement *self) {
    return self->radius;
}

TreeListElement *NodeListDecorator::parent_tree(NodeListElement *self) {
    return self->correspondingTree;
}

Coordinate NodeListDecorator::coordinate(NodeListElement *self) {
    return self->position;
}

int NodeListDecorator::mag(NodeListElement *self) {
    return self->createdInMag;
}

int NodeListDecorator::viewport(NodeListElement *self) {
    return self->createdInVp;
}

QString NodeListDecorator::static_NodeListElement_help() {
    return QString("A read-only class representing a tree node of the KNOSSOS skeleton. Access to attributes only via getter." \
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
void NodeListDecorator::set_node_id(NodeListElement *self, int node_id) {
    self->nodeID = node_id;
}

void NodeListDecorator::set_comment(NodeListElement *self, char *comment) {
    if(!self->comment) {
        self->comment = new commentListElement();
        self->comment->content = comment;
        self->comment->node = self;
    }
}

void NodeListDecorator::set_time(NodeListElement *self, int time) {
    self->timestamp = time;
}

void NodeListDecorator::set_radius(NodeListElement *self, float radius) {
    self->radius = radius;
}

void NodeListDecorator::set_coordinate(NodeListElement *self, int x, int y, int z) {
    self->position.x = x;
    self->position.y = y;
    self->position.z = z;
}

void NodeListDecorator::set_coordinate(NodeListElement *self, Coordinate coordinate) {
    self->position = coordinate;
}

void NodeListDecorator::set_viewport(NodeListElement *self, int viewport) {
    self->createdInVp = viewport;
}

void NodeListDecorator::set_mag(NodeListElement *self, int magnification) {
    self->createdInMag = magnification;
}

void NodeListDecorator::set_parent_tree(NodeListElement *self, TreeListElement *parent_tree) {
    self->correspondingTree = parent_tree;
}

void NodeListDecorator::set_parent_tree_id(NodeListElement *self, int id) {
    if(self->correspondingTree) {

    }
}
*/
