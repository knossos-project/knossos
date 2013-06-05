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

#include "commentswidget.h"
#include "GUIConstants.h"
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVariant>
#include "knossos-global.h"


extern struct stateInfo *state;

CommentsWidget::CommentsWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Comments Shortcuts");
    QFormLayout *layout = new QFormLayout();

    labels = new QLabel*[NUM];
    textFields = new QLineEdit*[NUM];

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

    loadSettings();
}

void CommentsWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(COMMENTS_WIDGET);
    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();
    this->textFields[0]->setText(settings.value(COMMENT1).toString());
    this->textFields[1]->setText(settings.value(COMMENT2).toString());
    this->textFields[2]->setText(settings.value(COMMENT3).toString());
    this->textFields[3]->setText(settings.value(COMMENT4).toString());
    this->textFields[4]->setText(settings.value(COMMENT1).toString());
    settings.endGroup();

    this->setGeometry(x, y, width, height);

}

void CommentsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(COMMENTS_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());
    settings.setValue(COMMENT1, this->textFields[0]->text());
    settings.setValue(COMMENT2, this->textFields[1]->text());
    settings.setValue(COMMENT3, this->textFields[2]->text());
    settings.setValue(COMMENT4, this->textFields[3]->text());
    settings.setValue(COMMENT5, this->textFields[4]->text());
    settings.endGroup();
}

void CommentsWidget::deleteComments() {
    int retValue = QMessageBox::warning(this, "Warning", "Do you really want to clear all comment boxes?", QMessageBox::Yes, QMessageBox::No);
    switch(retValue) {
        case QMessageBox::Yes:
            for(int i = 0; i < NUM; i++) {
                textFields[i]->clear();
            }

    }

}

void CommentsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit this->uncheckSignal();
}

/**
  * This method is a replacement for SIGNAL and SLOT. If the user pushs the return button in the respective textField, the method finds out which textField is
  * clicked and sets the corresponding comment in state->viewerState->gui->
  * @todo an optical feedback for the user would be a nice feature
  */
bool CommentsWidget::eventFilter(QObject *obj, QEvent *event) {
    for(int i = 0; i < NUM; i++) {
        if(textFields[i] == obj) {
            if(event->type() == QEvent::KeyPress) {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                if(keyEvent->key() == Qt::Key_Return) {
                    if(!textFields[i]->text().isEmpty()) {
                        if(i == 0) {
                            state->viewerState->gui->comment1 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 1) {
                            state->viewerState->gui->comment2 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 2) {
                            state->viewerState->gui->comment3 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 3) {
                            state->viewerState->gui->comment4 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 4) {
                            state->viewerState->gui->comment5 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}
