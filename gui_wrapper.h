#ifndef GUI_WRAPPER_H
#define GUI_WRAPPER_H

#include "skeleton/node.h"

void checkedMoveNodes(QWidget * parent, int treeID);
void checkedToggleNodeLink(QWidget * parent, nodeListElement & lhs, nodeListElement & rhs);

#endif//GUI_WRAPPER_H
