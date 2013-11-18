#include "commentshighlightingtab.h"

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

#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QDebug>
#include <QKeyEvent>
#include "GUIConstants.h"
#include "knossos-global.h"

extern struct stateInfo *state;
static const int N = 5;
static QMap<QString, color4F> colorMap;

CommentsHighlightingTab::CommentsHighlightingTab(QWidget *parent) :
    QWidget(parent)
{

    state->viewerState->gui->commentSubstr = new QStringList();
    state->viewerState->gui->commentColors = new char*[N];

    color4F color;
    SET_COLOR(color, 0.13, 0.69, 0.3, 1.);
    colorMap.insert(GREEN, color);
    SET_COLOR(color, 0.94, 0.89, 0.69, 1.);
    colorMap.insert(ROSE, color);
    SET_COLOR(color, 0.6, 0.85, 0.92, 1.);
    colorMap.insert(AZURE, color);
    SET_COLOR(color, 0.64, 0.29, 0.64, 1.);
    colorMap.insert(PURPLE, color);
    SET_COLOR(color, 0.73, 0.48, 0.34, 1.);
    colorMap.insert(BROWN, color);

    /*
    for(int i = 0; i < N; i++) {
        for(int k = 0; k < N; k++) {

        }
    }*/


    QVBoxLayout *layout = new QVBoxLayout();

    enableCondColoringCheckBox = new QCheckBox("Enable cond. coloring");
    enableCondRadiusCheckBox = new QCheckBox("Enable cond. radius");

    QHBoxLayout *subLayout = new QHBoxLayout();
    subLayout->addWidget(enableCondColoringCheckBox);
    subLayout->addWidget(enableCondRadiusCheckBox);
    layout->addLayout(subLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    descriptionLabel = new QLabel("Define substring, color and rendering radius");
    layout->addWidget(descriptionLabel);

    QGridLayout *gridLayout = new QGridLayout();

    numLabel = new QLabel*[N];
    substringFields = new QLineEdit*[N];
    colorComboBox = new QComboBox*[N];
    radiusSpinBox = new QDoubleSpinBox*[N];
    colorLabel = new QLabel*[N];
    radiusLabel = new QLabel*[N];

    for(int i = 0; i < N; i++) {

        state->viewerState->gui->commentSubstr->push_back(QString(""));
        substringFields[i] = new QLineEdit();
        substringFields[i]->setObjectName(QString("%1").arg(i));
        colorComboBox[i] = new QComboBox();
        colorComboBox[i]->addItems(QStringList(colorMap.keys()));

        connect(substringFields[i], SIGNAL(editingFinished()), this, SLOT(substringEntered()));
        connect(colorComboBox[i], SIGNAL(currentIndexChanged(QString)), this, SLOT(colorChanged(QString)));

        radiusSpinBox[i] = new QDoubleSpinBox();       
        radiusSpinBox[i]->setSingleStep(0.25);
        radiusSpinBox[i]->setMaximum(500000);
        connect(radiusSpinBox[i], SIGNAL(valueChanged(double)), this, SLOT(radiusChanged(double)));

        numLabel[i] = new QLabel(QString("%1").arg(i+1));        
        colorLabel[i] = new QLabel("color");
        radiusLabel[i] = new QLabel("radius");

        gridLayout->addWidget(numLabel[i], i, 1);
        gridLayout->addWidget(substringFields[i], i, 2);
        gridLayout->addWidget(colorLabel[i], i, 3);
        gridLayout->addWidget(colorComboBox[i], i, 4);
        gridLayout->addWidget(radiusLabel[i], i, 5);
        gridLayout->addWidget(radiusSpinBox[i], i , 6);
    }

    layout->addLayout(gridLayout);
    setLayout(layout);

    connect(enableCondColoringCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableCondColoringChecked(bool)));
    connect(enableCondRadiusCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableCondRadiusChecked(bool)));

    for(int i = 0; i < N; i++) {
        state->viewerState->gui->commentSubstr->replace(i, substringFields[i]->text());
    }

}

void CommentsHighlightingTab::enableCondColoringChecked(bool on) {
    if(on) {
        state->skeletonState->userCommentColoringOn = true;        
    } else {
        state->skeletonState->userCommentColoringOn = false;       
    }
}

void CommentsHighlightingTab::enableCondRadiusChecked(bool on) {
    if(on) {
        state->skeletonState->commentNodeRadiusOn = true;
        qDebug() << "enabled";
    } else {
        state->skeletonState->commentNodeRadiusOn = false;
        qDebug() << "disabled";
    }
}


void CommentsHighlightingTab::substringEntered() {
    QLineEdit *field = (QLineEdit*) sender();
    for(int i = 0; i < N; i++) {
        if(field == substringFields[i]) {
            state->viewerState->gui->commentSubstr->replace(i, substringFields[i]->text());
         }
    }
}


void CommentsHighlightingTab::colorChanged(QString color) {
    qDebug() << color;
    QComboBox *colorBox = (QComboBox*) sender();
    for(int i = 0; i < N; i++) {
        if(colorBox == colorComboBox[i]) {
            color4F col;
            if(color == "green") {
                SET_COLOR(state->skeletonState->commentColors[i], 0.13, 0.69, 0.3, 1.);
                qDebug() << "green selected";
            } else if(color == "rose") {
                 SET_COLOR(state->skeletonState->commentColors[i], 0.94, 0.89, 0.69, 1.);
                 qDebug() << " rose selected";
            } else if(color == "azure") {
                SET_COLOR(state->skeletonState->commentColors[i], 0.6, 0.85, 0.92, 1.);
                qDebug() << "azure selected";
            } else if(color == "purple") {
                SET_COLOR(state->skeletonState->commentColors[i], 0.64, 0.29, 0.64, 1.);
                qDebug() << "purple";
            } else if(color == "brown") {
                SET_COLOR(state->skeletonState->commentColors[i], 0.73, 0.48, 0.34, 1.);
                qDebug() << "brown";
            }
        }
    }
}

void CommentsHighlightingTab::radiusChanged(double value) {
    QDoubleSpinBox *spinBox = (QDoubleSpinBox*) sender();
    for(int i = 0; i < N; i++) {
        if(spinBox == radiusSpinBox[i]) {
            state->skeletonState->commentNodeRadii[i] = value;
        }
    }
}

