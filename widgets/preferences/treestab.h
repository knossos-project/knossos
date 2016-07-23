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

#ifndef TREESTAB_H
#define TREESTAB_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QWidget>

class TreesTab : public QWidget
{
    friend class PreferencesWidget;
    friend class ViewportBase;//hotkey 1 in vps – to toggle the skeleton overlay
    Q_OBJECT
    QGridLayout mainLayout;
    // tree render options
    QCheckBox highlightActiveTreeCheck{tr("Highlight active tree")};
    QCheckBox highlightIntersectionsCheck{tr("Highlight intersections")};
    QCheckBox lightEffectsCheck{tr("Enable light effects")};
    QCheckBox ownTreeColorsCheck{tr("Use custom tree colors")};
    QString lutFilePath;
    QPushButton loadTreeLUTButton{tr("Load …")};
    QLabel depthCutOffLabel{tr("Depth cutoff:")};
    QDoubleSpinBox depthCutoffSpin;
    QLabel renderQualityLabel{tr("Skeleton rendering quality:")};
    QComboBox renderQualityCombo;
    // tree visibility
    QRadioButton wholeSkeletonRadio{tr("Show whole skeleton")};
    QRadioButton selectedTreesRadio{tr("Show only selected trees")};
    QCheckBox skeletonInOrthoVPsCheck{tr("Show skeleton in Ortho VPs")};
    QCheckBox skeletonIn3DVPCheck{tr("Show skeleton in 3D VP")};

    void updateTreeDisplay();
    void loadTreeLUTButtonClicked(QString path = "");
    void saveSettings(QSettings & settings) const;
    void loadSettings(const QSettings & settings);
public:
    explicit TreesTab(QWidget *parent = 0);

signals:

public slots:
};

#endif // TREESTAB_H
