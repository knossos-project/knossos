/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef DATASETANDSEGMENTATIONTAB_H
#define DATASETANDSEGMENTATIONTAB_H

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

class DatasetAndSegmentationTab : public QWidget
{
    friend class PreferencesWidget;
    Q_OBJECT
    QGridLayout mainLayout;
    // dataset
    QLabel datasetHeader{"<strong>Dataset</strong>"};
    QFrame datasetSeparator;
    QCheckBox datasetLinearFilteringCheckBox{"Enable linear filtering"};
    QCheckBox useOwnDatasetColorsCheckBox{"Use own dataset colors"};
    QPushButton useOwnDatasetColorsButton{"Load …"};
    QString lutFilePath;
    QLabel datasetDynamicRangeLabel{"Dataset dynamic range"}, biasLabel{"Bias"}, rangeDeltaLabel{"Range delta"};
    QSpinBox biasSpinBox, rangeDeltaSpinBox;
    QSlider biasSlider{Qt::Horizontal}, rangeDeltaSlider{Qt::Horizontal};
    // segmentation overlay
    QLabel segmentationHeader{"<strong>Segmentation</strong>"};
    QFrame segmentationSeparator;
    QLabel segmentationOverlayLabel{"Overlay opacity"};
    QSpinBox segmentationOverlaySpinBox;
    QSlider segmentationOverlaySlider{Qt::Horizontal};
    // segmentation volume
    QCheckBox volumeRenderCheckBox{"Show volume in 3D viewport"};
    QLabel volumeOpaquenessLabel{"Volume opacity"};
    QSpinBox volumeOpaquenessSpinBox;
    QSlider volumeOpaquenessSlider{Qt::Horizontal};
    QLabel volumeColorLabel{"Volume background color"};
    QPushButton volumeColorButton;

    void useOwnDatasetColorsButtonClicked(QString path = "");
    void saveSettings(QSettings & settings) const;
    void loadSettings(const QSettings & settings);
public:
    explicit DatasetAndSegmentationTab(QWidget *parent = 0);

signals:
    void volumeRenderToggled();
public slots:
};

#endif // DATASETANDSEGMENTATIONTAB_H
