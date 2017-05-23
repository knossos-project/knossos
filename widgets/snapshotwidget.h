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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "viewports/viewport.h"
#include "widgets/DialogVisibilityNotify.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>

class SnapshotViewer : public QDialog {
    QGridLayout mainLayout;

    QLabel infoLabel;
    QScrollArea imageArea;
    QLabel imageLabel;
    QPushButton saveButton{tr("Save…")};
    QPushButton copyButton{tr("Copy to clipboard")};
    QPushButton cancelButton{tr("Cancel")};

    QString saveDir;

public:
    explicit SnapshotViewer(const QImage & image, const QString & info, const QString & saveDir, const QString & defaultFilename, QWidget * parent = 0);
    QString getSaveDir() const { return saveDir; }
};

class SnapshotWidget : public DialogVisibilityNotify {
    friend struct WidgetContainer;
    Q_OBJECT

    QVBoxLayout mainLayout;
    QComboBox sizeCombo;
    QButtonGroup vpGroup;
    QRadioButton vpXYRadio{"XY viewport"}, vpXZRadio{"XZ viewport"}, vpZYRadio{"ZY viewport"}, vpArbRadio{"Arb viewport"}, vp3dRadio{"3D viewport"};
    QCheckBox withAxesCheck{"Dataset axes"}, withBoxCheck{"Dataset box"}, withOverlayCheck{"Segmentation overlay"}, withSkeletonCheck{"Skeleton overlay"}, withScaleCheck{"Physical scale"}, withVpPlanes{"Viewport planes"};
    QPushButton snapshotButton{"Take snapshot"};

    QString saveDir{QDir::homePath()};
public:
    explicit SnapshotWidget(QWidget *parent = 0);
    void saveSettings();
    void loadSettings();
public slots:
    void openForVP(const ViewportType type);
    void updateOptionVisibility();
    void showViewer(const QImage & image);
signals:
    void snapshotVpSizeRequest(SnapshotOptions & options);
    void snapshotDatasetSizeRequest(SnapshotOptions & options);
    void snapshotRequest(const SnapshotOptions & options);
};

#endif // SNAPSHOTWIDGET_H
