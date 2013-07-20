#include "commentspreferencestab.h"

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
#include "GUIConstants.h"

static const int N = 5;

CommentsPreferencesTab::CommentsPreferencesTab(QWidget *parent) :
    QWidget(parent)
{

    QStringList list;
    list << GREEN << ROSE << AZURE << PURPLE << BROWN;

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

    numLabel = new QLabel*[5];
    substringFields = new QLineEdit*[N];
    colorComboBox = new QComboBox*[N];
    radiusSpinBox = new QDoubleSpinBox*[N];
    colorLabel = new QLabel*[N];
    radiusLabel = new QLabel*[N];

    for(int i = 0; i < N; i++) {
        substringFields[i] = new QLineEdit();
        colorComboBox[i] = new QComboBox();
        colorComboBox[i]->addItems(list);

        radiusSpinBox[i] = new QDoubleSpinBox();
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
}
