/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "gui_wrapper.h"

#include "session.h"
#include "skeleton/skeletonizer.h"

#include <QMessageBox>
#include <QObject>

void checkedMoveNodes(QWidget * parent, decltype(treeListElement::treeID) treeID) {
    QMessageBox prompt(parent);
    prompt.setIcon(QMessageBox::Question);
    prompt.setText(QObject::tr("Confirmation Requested"));
    prompt.setInformativeText(QObject::tr("Do you really want to move selected nodes to tree %1?").arg(treeID));
    QPushButton *confirmButton = prompt.addButton(QObject::tr("Move"), QMessageBox::AcceptRole);
    prompt.addButton(QMessageBox::Cancel);
    prompt.exec();
    if (prompt.clickedButton() == confirmButton) {
        Skeletonizer::singleton().moveSelectedNodesToTree(treeID);
    }
}

void checkedToggleNodeLink(QWidget * parent, nodeListElement & lhs, nodeListElement & rhs) {
    if (!Session::singleton().annotationMode.testFlag(AnnotationMode::SkeletonCycles) && Skeletonizer::singleton().areConnected(lhs, rhs)) {
        QMessageBox prompt(parent);
        prompt.setIcon(QMessageBox::Warning);
        prompt.setText(QObject::tr("Cycle detected!"));
        prompt.setInformativeText(QObject::tr("If you want to allow cycles, please select 'Advanced Tracing' in the dropdown menu in the toolbar."));
        prompt.exec();
        return;
    }
    Skeletonizer::singleton().toggleLink(lhs, rhs);
}
