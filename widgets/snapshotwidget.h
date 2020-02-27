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

#pragma once

#include "viewports/viewportbase.h"
#include "widgets/DialogVisibilityNotify.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class SnapshotViewer : public QDialog {
    QGridLayout mainLayout;

    QLabel infoLabel;
    class AspectRatioLabel : public QLabel {
    public:
        QPixmap pixmap;// intentionally conflict the name of pixmap() because that only returns the scaled pixmap
        AspectRatioLabel(QPixmap pixmap) : pixmap{pixmap} {
            setPixmap(pixmap);
            setMinimumSize(std::min(pixmap.width(), 400), std::min(pixmap.height(), 400));
        }
        virtual void resizeEvent(QResizeEvent *) override {
            setPixmap(pixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    };
    AspectRatioLabel imageLabel;
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
    QCheckBox withAxesCheck{"Dataset axes"}, withBoxCheck{"Dataset box"}, withMeshesCheck{"Mesh"}, withOverlayCheck{"Segmentation overlay"}, withScaleCheck{"Physical scale"}, withSkeletonCheck{"Skeleton overlay"}, withVpPlanes{"Viewport planes"};
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
