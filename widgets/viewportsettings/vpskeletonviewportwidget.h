#ifndef VPSKELETONVIEWPORTWIDGET_H
#define VPSKELETONVIEWPORTWIDGET_H

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
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class VPSkeletonViewportWidget : public QWidget
{
    Q_OBJECT

    friend class ViewportSettingsWidget;

    QVBoxLayout mainLayout;
    QGridLayout gridLayout;

    QLabel skeletonDisplayModesLabel{"Skeleton Display Modes"};
    QFrame line;
    QRadioButton wholeSkeletonRadioButton{"Whole Skeleton"};
    QRadioButton onlyCurrentCubeRadioButton{"Only Current Cube"};
    QRadioButton onlyActiveTreeRadioButton{"Only Active Tree"};
    QRadioButton hideSkeletonRadioButton{"Hide Skeleton (fast)"};
    QLabel datasetVisualizationLabel{"Dataset Visualization"};
    QFrame line2;
    QCheckBox showXYPlaneCheckBox{"Show XY Plane"};
    QCheckBox showXZPlaneCheckBox{"Show XZ Plane"};
    QCheckBox showYZPlaneCheckBox{"Show YZ Plane"};
    QLabel view3dlabel{"3D View"};
    QFrame line3;
    QCheckBox rotateAroundActiveNodeCheckBox{"Rotate Around Active Node"};

    void resetDisplayMode();
//slots
    void wholeSkeletonSelected();
    void onlyCurrentCubeSelected();
    void onlyActiveTreeSelected();
    void hideSkeletonSelected();
public:
    explicit VPSkeletonViewportWidget(QWidget * const parent = nullptr);
signals:
    void updateViewerStateSignal();
};

#endif // VPSKELETONVIEWPORTWIDGET_H
