#include "gui_wrapper.h"

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
