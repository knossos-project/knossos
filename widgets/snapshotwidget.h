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

#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "viewport.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class SnapshotWidget : public QDialog {
    Q_OBJECT
    QString saveDir;
    QComboBox sizeCombo;
    QRadioButton vpXYRadio{"XY viewport"}, vpXZRadio{"XZ viewport"}, vpZYRadio{"ZY viewport"}, vpArbRadio{"Arb viewport"}, vp3dRadio{"3D viewport"};
    QCheckBox withAxesCheck{"Dataset Axes"}, withOverlayCheck{"Segmentation overlay"}, withSkeletonCheck{"Skeleton overlay"}, withScaleCheck{"Physical scale"}, withVpPlanes{"Viewport planes"};
    QPushButton snapshotButton{"Take snapshot"};
    QVBoxLayout mainLayout;
    uint getCheckedViewport() const;
    QString defaultFilename() const;
public:
    explicit SnapshotWidget(QWidget *parent = 0);
    void saveSettings();
    void loadSettings();
public slots:
    void openForVP(const ViewportType type);
    void updateOptionVisibility();
signals:
    void visibilityChanged(bool);
    void snapshotRequest(const QString & path, const ViewportType vpType, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes);

private:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
};

#endif // SNAPSHOTWIDGET_H
