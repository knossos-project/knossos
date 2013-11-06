#include "commentshortcutstab.h"

/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include "GUIConstants.h"
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVariant>
#include <QMessageBox>
#include "knossos-global.h"

extern struct stateInfo *state;

CommentShortCutsTab::CommentShortCutsTab(QWidget *parent) :
    QWidget(parent)
{
    QFormLayout *layout = new QFormLayout();

    labels = new QLabel*[NUM];
    textFields = new QLineEdit*[NUM];

    /* this creates the Strings for the Label F1-F5 */
    for(int i = 0; i < NUM; i++) {
        QString tmp;
        tmp = QString("F%1").arg((i+1));
        labels[i] = new QLabel(tmp);

        textFields[i] = new QLineEdit();
        textFields[i]->installEventFilter(this);
        layout->addRow(labels[i], textFields[i]);
    }

    button = new QPushButton("Clear Comments Boxes");
    layout->addWidget(button);
    this->setLayout(layout);

    connect(button, SIGNAL(clicked()), this, SLOT(deleteComments()));

}

void CommentShortCutsTab::deleteCommentsWithoutConfirmation() {

    for(int i = 0; i < NUM; i++) {
        textFields[i]->clear();
    }
}

void CommentShortCutsTab::deleteComments() {
    QMessageBox warning;
    warning.setIcon(QMessageBox::Warning);
    warning.setWindowFlags(Qt::WindowStaysOnTopHint);
    warning.setWindowTitle("Warning");
    warning.setText("Do you really want to clear all comment boxes?");
    QPushButton *yes = warning.addButton("Yes", QMessageBox::AcceptRole);
    warning.addButton("No", QMessageBox::RejectRole);
    warning.exec();
    if(warning.clickedButton() == yes) {
        for(int i = 0; i < NUM; i++) {
            textFields[i]->clear();
        }
    }
}

/**
  * This method is a replacement for SIGNAL and SLOT. If the user pushs the return button in the respective textField, the method finds out which textField is
  * clicked and sets the corresponding comment in state->viewerState->gui->
  * @todo an optical feedback for the user would be a nice feature
  */
bool CommentShortCutsTab::eventFilter(QObject *obj, QEvent *event) {

    for(int i = 0; i < NUM; i++) {
        if(textFields[i] == obj) {
            if(event->type() == QEvent::KeyPress) {               
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                if(keyEvent->key() == Qt::Key_Return) {
                    if(!textFields[i]->text().isEmpty()) {
                        if(i == 0) {                            
                            strcpy(state->viewerState->gui->comment1, textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 1) {
                            strcpy(state->viewerState->gui->comment2, textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 2) {
                           strcpy(state->viewerState->gui->comment3, textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 3) {
                           strcpy(state->viewerState->gui->comment4, textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 4) {
                            strcpy(state->viewerState->gui->comment5, textFields[i]->text().toStdString().c_str());
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}
