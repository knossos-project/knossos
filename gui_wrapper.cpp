/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "gui_wrapper.h"

#include "session.h"
#include "skeleton/skeletonizer.h"

#include <QApplication>
#include <QMessageBox>
#include <QObject>

nodeListElement * checkedPopBranchNode() {
    if (Skeletonizer::singleton().skeletonState.branchpointUnresolved) {
        QMessageBox prompt{QApplication::activeWindow()};
        prompt.setIcon(QMessageBox::Question);
        prompt.setText(QObject::tr("Unresolved branch point. \nDo you really want to jump to the next one?"));
        prompt.setInformativeText(QObject::tr("No node has been added after jumping to the last branch point."));
        prompt.addButton(QObject::tr("Jump anyway"), QMessageBox::AcceptRole);
        auto * cancel = prompt.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);

        prompt.exec();
        if (prompt.clickedButton() == cancel) {
            return nullptr;
        }
    }
    auto * node = Skeletonizer::singleton().popBranchNode();
    if (node == nullptr) {
        QMessageBox box{QApplication::activeWindow()};
        box.setIcon(QMessageBox::Information);
        box.setText(QObject::tr("No branch points remain."));
        box.exec();
        qDebug() << box.text();
    }
    return node;
}

void checkedToggleNodeLink(nodeListElement & lhs, nodeListElement & rhs) {
    if (!Session::singleton().annotationMode.testFlag(AnnotationMode::SkeletonCycles) && Skeletonizer::singleton().areConnected(lhs, rhs)) {
        QMessageBox prompt{QApplication::activeWindow()};
        prompt.setIcon(QMessageBox::Warning);
        prompt.setText(QObject::tr("Cycle detected!"));
        prompt.setInformativeText(QObject::tr("If you want to allow cycles, please select 'Advanced Tracing' in the dropdown menu in the toolbar."));
        prompt.exec();
        return;
    }
    Skeletonizer::singleton().toggleLink(lhs, rhs);
}

void loadLutError(const QString & path) {
    QMessageBox lutErrorBox{QApplication::activeWindow()};
    lutErrorBox.setIcon(QMessageBox::Warning);
    lutErrorBox.setText(QObject::tr("LUT loading failed"));
    lutErrorBox.setInformativeText(QObject::tr("LUTs are restricted to 256 RGB tuples"));
    lutErrorBox.setDetailedText(QObject::tr("Path: %1").arg(path));
    lutErrorBox.exec();
}
