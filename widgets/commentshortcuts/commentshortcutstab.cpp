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

#include "../GUIConstants.h"
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVariant>
#include <QMessageBox>
#include "knossos-global.h"

extern  stateInfo *state;

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
        layout->addRow(labels[i], textFields[i]);
    }

    button = new QPushButton("Clear Comments Boxes");
    layout->addWidget(button);
    this->setLayout(layout);

    for(int i = 0; i < 5; i++) {
        connect(textFields[i], SIGNAL(textEdited(QString)), this, SLOT(commentChanged(QString)));
    }

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

void CommentShortCutsTab::commentChanged(QString comment) {
    qDebug() << comment;

    QObject *emitter = sender();

    if(textFields[0] == emitter) {
        strcpy(state->viewerState->gui->comment1, comment.toLocal8Bit().data());
    } else if(textFields[1] == emitter) {
       strcpy(state->viewerState->gui->comment2, comment.toLocal8Bit().data());
    } else if(textFields[2] == emitter) {
        strcpy(state->viewerState->gui->comment3, comment.toLocal8Bit().data());
     } else if(textFields[3] == emitter) {
        strcpy(state->viewerState->gui->comment4, comment.toLocal8Bit().data());
     } else if(textFields[4] == emitter) {
        strcpy(state->viewerState->gui->comment5, comment.toLocal8Bit().data());
     }
}

