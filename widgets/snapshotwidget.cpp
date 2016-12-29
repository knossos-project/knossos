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

#include "GuiConstants.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "snapshotwidget.h"
#include "stateInfo.h"
#include "viewer.h"

#include <math.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QSettings>

SnapshotWidget::SnapshotWidget(QWidget *parent) : DialogVisibilityNotify(SNAPSHOT_WIDGET, parent), saveDir(QDir::homePath()) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/snapshot.png"));
    setWindowTitle("Snapshot Tool");
    sizeCombo.addItem("8192 x 8192");
    sizeCombo.addItem("4096 x 4096");
    sizeCombo.addItem("2048 x 2048");
    sizeCombo.addItem("1024 x 1024");
    sizeCombo.setCurrentIndex(2); // 2048x2048 default

    auto viewportChoiceLayout = new QVBoxLayout();
    auto vpGroup = new QButtonGroup(this);
    vpGroup->addButton(&vpXYRadio);
    vpGroup->addButton(&vpXZRadio);
    vpGroup->addButton(&vpZYRadio);
    vpGroup->addButton(&vpArbRadio);
    vpGroup->addButton(&vp3dRadio);
    viewportChoiceLayout->addWidget(&vpXYRadio);
    viewportChoiceLayout->addWidget(&vpXZRadio);
    viewportChoiceLayout->addWidget(&vpZYRadio);
    viewportChoiceLayout->addWidget(&vpArbRadio);
    viewportChoiceLayout->addWidget(&vp3dRadio);
    QObject::connect(vpGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](const int) { updateOptionVisibility(); });

    auto imageOptionsLayout = new QVBoxLayout();
    imageOptionsLayout->addWidget(&withAxesCheck);
    imageOptionsLayout->addWidget(&withBoxCheck);
    imageOptionsLayout->addWidget(&withOverlayCheck);
    imageOptionsLayout->addWidget(&withSkeletonCheck);
    imageOptionsLayout->addWidget(&withScaleCheck);
    imageOptionsLayout->addWidget(&withVpPlanes);

    auto settingsLayout = new QHBoxLayout();
    settingsLayout->addLayout(viewportChoiceLayout);
    auto line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameStyle(QFrame::Sunken);
    settingsLayout->addWidget(line);
    settingsLayout->addLayout(imageOptionsLayout);

    mainLayout.addWidget(&sizeCombo);
    mainLayout.addLayout(settingsLayout);
    mainLayout.addWidget(&snapshotButton);

    QObject::connect(&snapshotButton, &QPushButton::clicked, [this]() {
        state->viewerState->renderInterval = SLOW;
        const auto path = QFileDialog::getSaveFileName(this, tr("Save path"), saveDir + defaultFilename(), tr("Images (*.png *.xpm *.xbm *.jpg *.bmp)"));
        state->viewerState->renderInterval = FAST;
        if(path.isEmpty() == false) {
            QFileInfo info(path);
            saveDir = info.absolutePath() + "/";
            const auto vp = vpXYRadio.isChecked() ? VIEWPORT_XY :
                            vpXZRadio.isChecked() ? VIEWPORT_XZ :
                            vpZYRadio.isChecked() ? VIEWPORT_ZY :
                            vpArbRadio.isChecked() ? VIEWPORT_ARBITRARY :
                                                    VIEWPORT_SKELETON;

            emit snapshotRequest(path, vp, 8192/pow(2, sizeCombo.currentIndex()), withAxesCheck.isChecked(), withBoxCheck.isChecked(), withOverlayCheck.isChecked(), withSkeletonCheck.isChecked(), withScaleCheck.isChecked(), withVpPlanes.isChecked());
        }
    });
    setLayout(&mainLayout);
}

void SnapshotWidget::openForVP(const ViewportType type) {
    vpXYRadio.setChecked(type == VIEWPORT_XY);
    vpXZRadio.setChecked(type == VIEWPORT_XZ);
    vpZYRadio.setChecked(type == VIEWPORT_ZY);
    vpArbRadio.setChecked(type == VIEWPORT_ARBITRARY);
    vp3dRadio.setChecked(type == VIEWPORT_SKELETON);
    updateOptionVisibility();
    show();
    activateWindow();
    raise();
}

uint SnapshotWidget::getCheckedViewport() const {
    return vpXYRadio.isChecked() ? VIEWPORT_XY :
           vpXZRadio.isChecked() ? VIEWPORT_XZ :
           vpZYRadio.isChecked() ? VIEWPORT_ZY :
           vpArbRadio.isChecked() ? VIEWPORT_ARBITRARY :
                                   VIEWPORT_SKELETON;
}

void SnapshotWidget::updateOptionVisibility() {
    withOverlayCheck.setVisible(vp3dRadio.isChecked() == false);
    withSkeletonCheck.setVisible(vp3dRadio.isChecked() == false || !Segmentation::singleton().volume_render_toggle);
    withScaleCheck.setVisible(vp3dRadio.isChecked() == false || !Segmentation::singleton().volume_render_toggle);
    withAxesCheck.setVisible(vp3dRadio.isChecked() && !Segmentation::singleton().volume_render_toggle);
    withBoxCheck.setVisible(vp3dRadio.isChecked() && !Segmentation::singleton().volume_render_toggle);
    withVpPlanes.setVisible(vp3dRadio.isChecked() && !Segmentation::singleton().volume_render_toggle);
}

QString SnapshotWidget::defaultFilename() const {
    const QString vp = vpXYRadio.isChecked() ? "XY" :
                   vpXZRadio.isChecked() ? "XZ" :
                   vpZYRadio.isChecked() ? "ZY" :
                   vpArbRadio.isChecked() ? "Arb" :
                                           "3D";
    auto pos = &state->viewerState->currentPosition;
    auto annotationName = Session::singleton().annotationFilename;
    annotationName.remove(0, annotationName.lastIndexOf("/") + 1); // remove directory structure from name
    annotationName.chop(annotationName.endsWith(".k.zip") ? 6 : /* .nml */ 4); // remove file type
    return QString("%1_%2_%3_%4_%5.png").arg(vp).arg(pos->x).arg(pos->y).arg(pos->z).arg(annotationName.isEmpty() ? state->name : annotationName);
}

void SnapshotWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(SNAPSHOT_WIDGET);
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(VIEWPORT, getCheckedViewport());
    settings.setValue(WITH_AXES, withAxesCheck.isChecked());
    settings.setValue(WITH_BOX, withBoxCheck.isChecked());
    settings.setValue(WITH_OVERLAY, withOverlayCheck.isChecked());
    settings.setValue(WITH_SKELETON, withSkeletonCheck.isChecked());
    settings.setValue(WITH_SCALE, withScaleCheck.isChecked());
    settings.setValue(WITH_VP_PLANES, withVpPlanes.isChecked());
    settings.setValue(SAVE_DIR, saveDir);
    settings.endGroup();
}

void SnapshotWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(SNAPSHOT_WIDGET);

    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    const auto vp = settings.value(VIEWPORT, VIEWPORT_XY).toInt();
    switch(vp) {
        case VIEWPORT_XY: vpXYRadio.setChecked(true); break;
        case VIEWPORT_XZ: vpXZRadio.setChecked(true); break;
        case VIEWPORT_ZY: vpZYRadio.setChecked(true); break;
        case VIEWPORT_ARBITRARY: vpArbRadio.setChecked(true); break;
        default: vp3dRadio.setChecked(true); break;
    }
    withAxesCheck.setChecked(settings.value(WITH_AXES, true).toBool());
    withBoxCheck.setChecked(settings.value(WITH_BOX, false).toBool());
    withOverlayCheck.setChecked(settings.value(WITH_OVERLAY, true).toBool());
    withSkeletonCheck.setChecked(settings.value(WITH_SKELETON, true).toBool());
    withScaleCheck.setChecked(settings.value(WITH_SCALE, true).toBool());
    withVpPlanes.setChecked(settings.value(WITH_VP_PLANES, false).toBool());
    updateOptionVisibility();
    saveDir = settings.value(SAVE_DIR, QDir::homePath() + "/").toString();

    settings.endGroup();
}
