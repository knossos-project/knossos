#include "gui_wrapper.h"

#include "session.h"
#include "skeleton/skeletonizer.h"

#include <QMessageBox>
#include <QObject>

void checkedMoveNodes(QWidget * parent, int treeID) {
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
