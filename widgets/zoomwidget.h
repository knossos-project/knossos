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
 *  or contact knossosteam@gmail.com
 */

#ifndef ZOOMWIDGET_H
#define ZOOMWIDGET_H

#include "widgets/DialogVisibilityNotify.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

// a horizontal slider with ticks and mag labels at each mag switch position
class ZoomSlider : public QSlider {
    Q_OBJECT
    using QSlider::setTickPosition; // prevent default tick painting in parent.
protected:
    virtual void paintEvent(QPaintEvent *ev) override;
public:
    int highestMag{0};
    int numTicks{0};
};

class ZoomWidget : public DialogVisibilityNotify {
    friend class MainWindow;
    Q_OBJECT

    float lastZoomSkel;
    bool userZoomSkel;

    QVBoxLayout mainLayout;
    QGridLayout zoomLayout;
    QFrame separator;
    // zoom section
    QLabel orthogonalDataViewportLabel{tr("Orthogonal Viewports")};
    QLabel skeletonViewportLabel{tr("Skeleton Viewport")};
    QDoubleSpinBox skeletonViewportSpinBox;
    QDoubleSpinBox orthoZoomSpinBox;
    ZoomSlider orthoZoomSlider;
    QPushButton zoomDefaultsButton{tr("All Zoom defaults")};
    // multires section
    QLabel multiresSectionLabel{tr("Magnification Settings")};
    QCheckBox lockDatasetCheckBox{tr("Lock dataset to current mag")};
    QLabel currentActiveMagDatasetLabel;
    QLabel highestActiveMagDatasetLabel;
    QLabel lowestActiveMagDatasetLabel;
    void applyZoom(const float newScreenPxXPerDataPx);
    void reinitializeOrthoZoomWidgets();
    float zoomStep{1};
    void updateOrthogonalZoomSpinBox();
    void updateOrthogonalZoomSlider();
public:
    explicit ZoomWidget(QWidget *, class DatasetLoadWidget * datasetLoadWidget);
public slots:
    void zoomDefaultsClicked();
    void update();
    void loadSettings();
    void saveSettings();
};

#endif // ZOOMWIDGET_H
