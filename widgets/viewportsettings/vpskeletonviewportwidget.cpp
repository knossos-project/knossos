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
#include "vpskeletonviewportwidget.h"

#include "skeleton/skeletonizer.h"
#include "segmentation/segmentation.h"

#include <QColorDialog>

VPSkeletonViewportWidget::VPSkeletonViewportWidget(QWidget * const parent) : QWidget(parent) {
    line.setFrameShape(QFrame::HLine);
    line.setFrameShadow(QFrame::Sunken);

    line2.setFrameShape(QFrame::HLine);
    line2.setFrameShadow(QFrame::Sunken);

    line3.setFrameShape(QFrame::HLine);
    line3.setFrameShadow(QFrame::Sunken);

    VolumeColorBox.setStyleSheet("background-color : " + Segmentation::singleton().volume_background_color.name() + ";");
    VolumeColorLayout.addWidget(&VolumeColorLabel);
    VolumeColorLayout.addWidget(&VolumeColorBox, 0, Qt::AlignRight);

    VolumeOpaquenessSpinBox.setMaximum(255);
    VolumeOpaquenessSpinBox.setSingleStep(1);
    VolumeOpaquenessSlider.setMaximum(255);
    VolumeOpaquenessSlider.setSingleStep(1);
    VolumeOpaquenessLayout.addWidget(&VolumeOpaquenessLabel);
    VolumeOpaquenessLayout.addWidget(&VolumeOpaquenessSlider);
    VolumeOpaquenessLayout.addWidget(&VolumeOpaquenessSpinBox);

    int currentGridColumn = 0;
    gridLayout.addWidget(&datasetVisualizationLabel, currentGridColumn++, 0);
    gridLayout.addWidget(&line2, currentGridColumn++, 0);
    gridLayout.addWidget(&showXYPlaneCheckBox, currentGridColumn++, 0);
    gridLayout.addWidget(&showXZPlaneCheckBox, currentGridColumn++, 0);
    gridLayout.addWidget(&showYZPlaneCheckBox, currentGridColumn++, 0);

    gridLayout.addWidget(&view3dlabel, currentGridColumn++, 0);
    gridLayout.addWidget(&line3, currentGridColumn++, 0);
    gridLayout.addWidget(&rotateAroundActiveNodeCheckBox, currentGridColumn++, 0);
    gridLayout.addWidget(&VolumeRenderFlagCheckBox, currentGridColumn++, 0);
    gridLayout.addLayout(&VolumeOpaquenessLayout, currentGridColumn++, 0);
    gridLayout.addLayout(&VolumeColorLayout, currentGridColumn++, 0);

    mainLayout.addLayout(&gridLayout);
    mainLayout.addStretch(1);//expand vertically with empty space
    setLayout(&mainLayout);

    QObject::connect(&showXYPlaneCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->showXYplane = checked; });
    QObject::connect(&showYZPlaneCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->showYZplane = checked; });
    QObject::connect(&showXZPlaneCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->showXZplane = checked; });
    QObject::connect(&rotateAroundActiveNodeCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->rotateAroundActiveNode = checked; });
    QObject::connect(&VolumeRenderFlagCheckBox, &QCheckBox::clicked, [](bool checked){Segmentation::singleton().volume_render_toggle = checked; });
    QObject::connect(&VolumeColorBox, &QPushButton::clicked, [&](){
        auto color = QColorDialog::getColor(Segmentation::singleton().volume_background_color, this, "Select background color");
        if (color.isValid() == QColorDialog::Accepted) {
            Segmentation::singleton().volume_background_color = color;
            VolumeColorBox.setStyleSheet("background-color : " + color.name() + ";");
        }
    });
    QObject::connect(&VolumeOpaquenessSlider, &QSlider::valueChanged, [&](int value){
        VolumeOpaquenessSpinBox.setValue(value);
        Segmentation::singleton().volume_opacity = value;
        Segmentation::singleton().volume_update_required = true;
    });
    QObject::connect(&VolumeOpaquenessSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
        VolumeOpaquenessSlider.setValue(value);
        Segmentation::singleton().volume_opacity = value;
        Segmentation::singleton().volume_update_required = true;
    });
}
