#include "skeletonproxy.h"
#include "functions.h"
#include "skeletonizer.h"
#include <QApplication>
#include <QFile>

extern stateInfo *state;

SkeletonProxySignalDelegate *signalDelegate = new SkeletonProxySignalDelegate();

SkeletonProxy::SkeletonProxy(QObject *parent) :
    QObject(parent)
{

}

int SkeletonProxy::skeleton_time() {
    return state->skeletonState->skeletonTime;
}

QString SkeletonProxy::skeleton_file() {
    return state->skeletonState->skeletonFileAsQString;
}

void SkeletonProxy::from_xml(const QString &filename) {
   if(!signalDelegate->loadSkeleton(filename)) {
       emit echo(QString("could not load from %1").arg(filename));
   }
}

void SkeletonProxy::to_xml(const QString &filename) {
    if(!signalDelegate->saveSkeleton(filename)) {
        emit echo(QString("could not save to %1").arg(filename));
    }
}

treeListElement *SkeletonProxy::first_tree() {
    return state->skeletonState->firstTree;
}

void SkeletonProxy::export_converter(const QString &path) {
    QDir dir(path);
    if(!dir.exists()) {
        emit echo("path does not exist");
        return;
    }

    QFile targetFile(path + "converter.py");
    if(!targetFile.open(QIODevice::WriteOnly)) {
        emit echo("error creating a file in this path");
        return;
    }

    QTextStream stream(&targetFile);

    QFile converter(":/misc/python/converter.py");
    if(!converter.open(QIODevice::ReadOnly)) {
        emit echo("error while reading the converter from the resource directory");
        return;
    }

    stream << converter.readAll();

    converter.close();
    targetFile.close();

    emit echo("exported to (" + targetFile.fileName() + ")");

}

bool SkeletonProxy::has_unsaved_changes() {
    return state->skeletonState->unsavedChanges;
}

void SkeletonProxy::delete_tree(int tree_id) {
   if(!Skeletonizer::delTree(tree_id)) {
       emit echo(QString("could not delete the tree with id %1").arg(tree_id));
   } else {
       emit signalDelegate->updateTreeViewSignal();
   }
}

void SkeletonProxy::delete_skeleton() {
    emit signalDelegate->clearSkeletonSignal();
}

void SkeletonProxy::set_active_node(int node_id) {
    if(!Skeletonizer::setActiveNode( 0, node_id)) {
        emit echo(QString("could not set the node with id %1 to active node").arg(node_id));
    }
}

nodeListElement *SkeletonProxy::active_node() {
    return state->skeletonState->activeNode;
}

void SkeletonProxy::add_node(int node_id, int x, int y, int z, int parent_tree_id, float radius, int inVp, int inMag, int time) {
    if(!checkNodeParameter(node_id, x, y, z)) {
        emit echo(QString("one of the first four arguments is in negative range. node is rejected"));
        return;
    }

    if(inVp < VIEWPORT_XY or inVp > VIEWPORT_ARBITRARY) {
        emit echo(QString("viewport argument is out of range. node is rejected"));
        return;
    }

    if(!state->skeletonState->activeTree) {
        this->add_tree(1);
    }


    if(parent_tree_id == 0) {
        parent_tree_id = state->skeletonState->activeTree ? (state->skeletonState->activeTree->treeID) : 1;
    }

    Coordinate coordinate(x, y, z);
    if(Skeletonizer::addNode(node_id, radius, parent_tree_id, &coordinate, inVp, inMag, time, false)) {
        Skeletonizer::setActiveNode( state->skeletonState->activeNode, node_id);
        emit signalDelegate->nodeAddedSignal();
    } else {
        emit echo(QString("could not add the node with node id %1").arg(node_id));
    }
}

QList<treeListElement *> *SkeletonProxy::trees() {
    QList<treeListElement *> *trees = new QList<treeListElement *>();
    treeListElement *currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        trees->append(currentTree);
        currentTree = currentTree->next;
    }
    return trees;
}

void SkeletonProxy::add_tree(int tree_id, const QString &comment, float r, float g, float b, float a) {
    color4F color(r, g, b, a);
    treeListElement *theTree = Skeletonizer::addTreeListElement(tree_id, color);
    if(!theTree) {
        emit echo(QString("could not add the tree with tree id %1").arg(tree_id));
        return;
    }

    if(comment.isEmpty() == false) {
        Skeletonizer::addTreeComment( tree_id, comment.toLocal8Bit().data());
    }

    Skeletonizer::setActiveTreeByID(tree_id);
    emit signalDelegate->updateToolsSignal();
    emit signalDelegate->treeAddedSignal(theTree);

}

void SkeletonProxy::add_comment(int node_id, char *comment) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(node_id);
    if(node) {
        if(!Skeletonizer::addComment( QString(comment), node, 0)) {
            emit echo(QString("An unexpected error occured while adding a comment for node id %1").arg(node_id));
        }
    } else {
        emit echo(QString("no node id id %1 found").arg(node_id));
    }
}

void SkeletonProxy::add_segment(int source_id, int target_id) {
    if(Skeletonizer::addSegment(source_id, target_id)) {

    } else {
       emit echo(QString("could not add a segment with source id %1 and target id %2").arg(source_id).arg(target_id));
    }
}

void SkeletonProxy::add_branch_node(int node_id) {
    nodeListElement *currentNode = Skeletonizer::findNodeByNodeID(node_id);
    if(currentNode) {
        if(Skeletonizer::pushBranchNode(true, false, currentNode, 0)) {
            emit signalDelegate->updateToolsSignal();
        } else {
            emit echo(QString("An unexpected error occured while adding a branch node"));
        }

    } else {
        emit echo(QString("no node with id %1 found").arg(node_id));
    }

}


QList<int> *SkeletonProxy::cube_data_at(int x, int y, int z) {
    Coordinate position(x / state->cubeEdgeLength, y / state->cubeEdgeLength, z / state->cubeEdgeLength);
    Byte *data = Hashtable::ht_get(state->Dc2Pointer[(int)std::log2(state->magnification)], position);

    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return 0;
    }

     QList<int> *resultList = new QList<int>();

    for(int i = 0; i < state->cubeBytes; i++) {
        resultList->append((int) data[i]);
    }

    return resultList;
}

void SkeletonProxy::render_mesh(mesh *mesh) {
    if(!mesh) {
        emit echo("Null objects can't be rendered ... nothing to do");
        return;
    }

    if(mesh->colsIndex == 0) {
        emit echo("Can't render a mesh without color");
    }

    if(mesh->vertsIndex == 0) {
        emit echo("Can't render a mesh without vertices");
        return;        
    }

    // it's ok if no normals are passed. This has to be checked in render method anyway
    if(mesh->mode < GL_POINTS or mesh->mode > GL_POLYGON) {
        emit echo("mesh contains an unknown vertex mode");
        return;

    }

    // lots of additional checks could be done
    state->skeletonState->userGeometry->append(mesh);
}


void SkeletonProxy::save_sys_path(const QString &path) {
    QFileInfo info(path);
    if(!info.exists()) {
        emit echo("No valid file path. Please check it again.");
        return;
    }

    emit signalDelegate->saveSettingsSignal("sys_path", path);
}

void SkeletonProxy::save_working_directory(const QString &path) {
    QFileInfo info(path);
    if(!info.exists()) {
        emit echo("No valid file path. Please check it again");
        return;
    }
   emit signalDelegate->saveSettingsSignal("working_dir", path);
}

QList<mesh *> *SkeletonProxy::user_geom_list() {
    return state->skeletonState->userGeometry;
 }

void SkeletonProxy::move_to(int x, int y, int z) {
    emit signalDelegate->userMoveSignal(x, y, z);
}


void SkeletonProxy::render_text(const QString &string, int x, int y, int z) {
    emit signalDelegate->renderTextSignal(new Coordinate(x, y, z), string.toLocal8Bit().data(), 0, 0);

}

void SkeletonProxy::loadStyleSheet(const QString &filename) {
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        emit echo("Error readint the style sheet file");
    }

    QString design(file.readAll());

    qApp->setStyleSheet(design);
    file.close();
}

QString SkeletonProxy::help() {
    return QString("This is the unique main interface between python and knossos. You can't create a separate instance of this object:" \
                   "\n\n GETTER:" \
                   "\n trees() : returns a list of trees" \
                   "\n active_node() : returns the active node object" \
                   "\n skeleton_file() : returns the file from which the current skeleton is loaded" \
                   "\n first_tree() : returns the first tree of the knossos skeleton" \
                   "\n export_converter(path) : creates a python class in the path which can be used to convert between the NewSkeleton class and KNOSSOS."
                   "\n\n SETTER:" \
                   "\n add_branch_node(node_id) : sets the node with node_id to branch_node" \
                   "\n add_segment(source_id, target_id) : adds a segment for the nodes. Both nodes must be added before"
                   "\n add_comment(node_id) : adds a comment for the node. Must be added before" \
                   "\n add_tree(tree_id, comment, r (opt), g (opt), b (opt), a (opt)) : adds a new tree" \
                   "\n\t If does not mind if no color is specified. The lookup table sets this automatically." \
                   "\n\n add_node(node_id, x, y, z, parent_id (opt), radius (opt), viewport (opt), mag (opt), time (opt))" \
                   "\n\t adds a node for a parent tree where a couple of parameter are optional. " \
                   "\n\t if no parent_id is set then the current active node will be chosen." \
                   "\n delete_tree(tree_id) : deletes the tree with the passed id. Returns a message if no such tree exists." \
                   "\n delete_skeleton() : deletes the entire skeleton." \
                   "\n from_xml(filename) : loads a skeleton from a .nml file" \
                   "\n to_xml(filename) : saves a skeleton to a .nml file" \
                   "\n cube_data_at(x, y, z) : returns the data cube at the viewport position (x, y, z) as a string containing 128 * 128 * 128 bytes (2MB) of grayscale values. " \
                   "\n render_Mesh(mesh) : renders the mesh. Call mesh.help() for additional information." \
                   "\n save_sys_path(path) : saves the python sys_path from the console" \
                   "\n move_to(x, y, z) : recenters the viewport coordinates to (x, y, z)" \
                   "\n save_working_directory(path) : saves the working directory from the console");
}
