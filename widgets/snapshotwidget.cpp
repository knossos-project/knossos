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

#include "snapshotwidget.h"

#include "annotation/annotation.h"
#include "dataset.h"
#include "GuiConstants.h"
#include "segmentation/segmentation.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardItemModel>

#include <boost/optional.hpp>
#include <math.h>

SnapshotViewer::SnapshotViewer(const QImage & image, const QString & info, const QString & saveDir, const QString &defaultFilename, QWidget *parent)
        : QDialog(parent), imageLabel{QPixmap::fromImage(image)}, saveDir(saveDir) {
    setWindowTitle("Snapshot Preview");

    setWindowFlag(Qt::CustomizeWindowHint, true);
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);

    infoLabel.setText(info);
    infoLabel.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);// don’t steal vertical space
    int row = 0;
    mainLayout.addWidget(&infoLabel, row++, 0, 1, 3);
    mainLayout.addWidget(&imageLabel, row++, 0, 1, 3);
    mainLayout.addWidget(&cancelButton, row, 0, Qt::AlignLeft);
    mainLayout.addWidget(&copyButton, row, 1);
    mainLayout.addWidget(&saveButton, row, 2);

    setLayout(&mainLayout);

    resize(minimumSize());// remove unnecessary spacing

    QObject::connect(&copyButton, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setPixmap(imageLabel.pixmap, QClipboard::Clipboard);
    });
    QObject::connect(&cancelButton, &QPushButton::clicked, this, &QDialog::close);
    QObject::connect(&saveButton, &QPushButton::clicked, [&defaultFilename, this]() {
        const auto path = state->viewer->suspend([&defaultFilename, this]{
            return QFileDialog::getSaveFileName(this, tr("Save path"), this->saveDir + defaultFilename, tr("Images (*.png *.xpm *.xbm *.jpg *.bmp)"));
        });
        if (path.isEmpty() == false) {
            QFileInfo info(path);
            this->saveDir = info.absolutePath() + "/";
            setCursor(Qt::BusyCursor);
            if (!imageLabel.pixmap.save(path)) {
                QMessageBox errorMsg{QApplication::activeWindow()};
                errorMsg.setIcon(QMessageBox::Critical);
                errorMsg.setText(tr("Saving snapshot has failed, probably because of missing write permission for path %1").arg(this->saveDir));
                errorMsg.exec();
            } else {
                close();
            }
        }
        setCursor(Qt::ArrowCursor);
    });
}

SnapshotWidget::SnapshotWidget(QWidget *parent) : DialogVisibilityNotify(SNAPSHOT_WIDGET, parent) {
    setWindowIcon(QIcon(":/resources/icons/snapshot.png"));
    setWindowTitle("Snapshot Tool");
    sizeCombo.addItem("Viewport Size");
    sizeCombo.addItem("Original Dataset Size");
    sizeCombo.addItem("1024 x 1024");
    sizeCombo.addItem("2048 x 2048");
    sizeCombo.addItem("4096 x 4096");
    sizeCombo.addItem("8192 x 8192");
    sizeCombo.setCurrentIndex(4); // 2048x2048 default


    vpArbRadio.setHidden(true);
    auto viewportChoiceLayout = new QVBoxLayout();
    vpGroup.addButton(&vpXYRadio, VIEWPORT_XY);
    vpGroup.addButton(&vpXZRadio, VIEWPORT_XZ);
    vpGroup.addButton(&vpZYRadio, VIEWPORT_ZY);
    vpGroup.addButton(&vpArbRadio, VIEWPORT_ARBITRARY);
    vpGroup.addButton(&vp3dRadio, VIEWPORT_SKELETON);
    viewportChoiceLayout->addWidget(&vpXYRadio);
    viewportChoiceLayout->addWidget(&vpXZRadio);
    viewportChoiceLayout->addWidget(&vpZYRadio);
    viewportChoiceLayout->addWidget(&vpArbRadio);
    viewportChoiceLayout->addWidget(&vp3dRadio);
    viewportChoiceLayout->setAlignment(Qt::AlignTop); // ensures constant spacing between radio buttons

    QObject::connect(&vpGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](const int index) {
        bool noOrigDataSize = (static_cast<ViewportType>(index) == VIEWPORT_SKELETON); // original dataset size does not make sense for skeleton vp
        QStandardItem & item = *static_cast<QStandardItemModel &>(*sizeCombo.model()).item(1);
        item.setFlags(noOrigDataSize ? item.flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled) : Qt::ItemIsSelectable|Qt::ItemIsEnabled); // disable
        item.setData(noOrigDataSize ? sizeCombo.palette().color(QPalette::Disabled, QPalette::Text) : QVariant() /* reset to default color */, Qt::TextColorRole); // grey out
        updateOptionVisibility();
    });

    auto imageOptionsLayout = new QVBoxLayout();
    imageOptionsLayout->addWidget(&withOverlayCheck, Qt::AlignTop);
    imageOptionsLayout->addWidget(&withSkeletonCheck, Qt::AlignTop);
    imageOptionsLayout->addWidget(&withMeshesCheck, Qt::AlignTop);
    imageOptionsLayout->addWidget(&withScaleCheck, Qt::AlignTop);
    imageOptionsLayout->addWidget(&withAxesCheck, Qt::AlignTop);
    imageOptionsLayout->addWidget(&withBoxCheck, Qt::AlignTop);
    imageOptionsLayout->addWidget(&withVpPlanes, Qt::AlignTop);
    imageOptionsLayout->setAlignment(Qt::AlignTop);

    auto settingsLayout = new QHBoxLayout();
    settingsLayout->addLayout(viewportChoiceLayout);
    auto line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameStyle(QFrame::Sunken);
    settingsLayout->addWidget(line);
    settingsLayout->addLayout(imageOptionsLayout);

    mainLayout.addWidget(&sizeCombo, 0, Qt::AlignTop);
    mainLayout.addLayout(settingsLayout);
    mainLayout.addWidget(&snapshotButton, 0, Qt::AlignBottom);

    QObject::connect(&snapshotButton, &QPushButton::clicked, [this]() {
        const auto vp = static_cast<ViewportType>(vpGroup.checkedId());
        SnapshotOptions options{boost::none, vp, 0, withAxesCheck.isChecked(), withBoxCheck.isChecked(), withMeshesCheck.isChecked(), withOverlayCheck.isChecked(), withScaleCheck.isChecked(), withSkeletonCheck.isChecked(), withVpPlanes.isChecked()};
        if (sizeCombo.currentIndex() == 0) {
            emit snapshotVpSizeRequest(options);
        } else if (sizeCombo.currentIndex() == 1) {
            emit snapshotDatasetSizeRequest(options);
        } else {
            options.size = std::pow(2, 10 + sizeCombo.currentIndex() - 2); // offset of 2 for special cases
            emit snapshotRequest(options);
        }
    });

    QObject::connect(state->viewer, &Viewer::enabledArbVP, [this](const bool on) {
        vpArbRadio.setVisible(on);
        if (vpArbRadio.isChecked() && vpArbRadio.isHidden()) {
            for (auto * button : vpGroup.buttons()) {
                if (button->isVisible()) {
                    button->setChecked(true);
                }
            }
        }
    });
    QObject::connect(state->viewer, &Viewer::changedDefaultVPSizeAndPos, [this]() {
        const auto xy = state->viewer->window->viewportXY->isVisibleTo(state->viewer->window);
        const auto xz = state->viewer->window->viewportXZ->isVisibleTo(state->viewer->window);
        const auto zy = state->viewer->window->viewportZY->isVisibleTo(state->viewer->window);
        const auto arb = state->viewer->window->viewportArb->isVisibleTo(state->viewer->window);
        const auto skel = state->viewer->window->viewport3D->isVisibleTo(state->viewer->window);
        vpXYRadio.setVisible(xy);
        vpXZRadio.setVisible(xz);
        vpZYRadio.setVisible(zy);
        vpArbRadio.setVisible(arb);
        vp3dRadio.setVisible(skel);
        if (vpGroup.checkedButton() && vpGroup.checkedButton()->isHidden()) {
            for (auto * button : vpGroup.buttons()) {
                if (button->isVisible()) {
                    button->setChecked(true);
                }
            }
        }
        snapshotButton.setEnabled(xy || xz || zy || arb || skel);
    });

    QObject::connect(&Segmentation::singleton(), &Segmentation::volumeRenderToggled, [this] (const bool) { updateOptionVisibility(); });

    setLayout(&mainLayout);
    setMinimumSize(310, 280);
    resize(minimumSize());
    setMaximumSize(minimumSize());
}

void SnapshotWidget::openForVP(const ViewportType type) {
    vpGroup.button(type)->setChecked(true);
    updateOptionVisibility();
    show();
    activateWindow();
    raise();
}

void SnapshotWidget::updateOptionVisibility() {
    withAxesCheck.setEnabled(vp3dRadio.isChecked() && !Segmentation::singleton().volume_render_toggle);
    withBoxCheck.setEnabled(vp3dRadio.isChecked() && !Segmentation::singleton().volume_render_toggle);
    withMeshesCheck.setEnabled(!Segmentation::singleton().volume_render_toggle);
    withOverlayCheck.setEnabled(vp3dRadio.isChecked() == false || !Segmentation::singleton().volume_render_toggle);
    withScaleCheck.setEnabled(vp3dRadio.isChecked() == false || !Segmentation::singleton().volume_render_toggle);
    withSkeletonCheck.setEnabled(vp3dRadio.isChecked() == false || !Segmentation::singleton().volume_render_toggle);
    withVpPlanes.setEnabled(vp3dRadio.isChecked() && !Segmentation::singleton().volume_render_toggle);
}

void SnapshotWidget::showViewer(const QImage & image) {
    const QString vp = vpXYRadio.isChecked() ? "XY" :
                   vpXZRadio.isChecked() ? "XZ" :
                   vpZYRadio.isChecked() ? "ZY" :
                   vpArbRadio.isChecked() ? "Arb" :
                                           "3D";
    auto pos = &state->viewerState->currentPosition;
    auto Annotationame = Annotation::singleton().annotationFilename;
    Annotationame.remove(0, Annotationame.lastIndexOf("/") + 1); // remove directory structure from name
    Annotationame.chop(Annotationame.endsWith(".k.zip") ? 6 : /* .nml */ 4); // remove file type
    const auto info = tr("%1 Viewport (%2)").arg(vp).arg(sizeCombo.currentText());
    const auto defaultFilename = QString("%1_%2_%3_%4_%5.png").arg(vp).arg(pos->x).arg(pos->y).arg(pos->z).arg(Annotationame.isEmpty() ? Dataset::current().experimentname : Annotationame);
    SnapshotViewer previewer(image, info, saveDir, defaultFilename, this);
    previewer.exec();
    saveDir = previewer.getSaveDir();
}

void SnapshotWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(SNAPSHOT_WIDGET);
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(SAVE_DIR, saveDir);
    settings.setValue(WITH_AXES, withAxesCheck.isChecked());
    settings.setValue(WITH_BOX, withBoxCheck.isChecked());
    settings.setValue(WITH_MESHES, withMeshesCheck.isChecked());
    settings.setValue(WITH_OVERLAY, withOverlayCheck.isChecked());
    settings.setValue(WITH_SCALE, withScaleCheck.isChecked());
    settings.setValue(WITH_SKELETON, withSkeletonCheck.isChecked());
    settings.setValue(WITH_VP_PLANES, withVpPlanes.isChecked());
    settings.setValue(VIEWPORT, vpGroup.checkedId());
    settings.endGroup();
}

void SnapshotWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(SNAPSHOT_WIDGET);

    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    saveDir = settings.value(SAVE_DIR, QDir::homePath() + "/").toString();
    const auto vp = settings.value(VIEWPORT, VIEWPORT_XY).toInt();
    vpGroup.button(vp)->setChecked(true);
    vpGroup.buttonClicked(vp);
    withAxesCheck.setChecked(settings.value(WITH_AXES, true).toBool());
    withBoxCheck.setChecked(settings.value(WITH_BOX, false).toBool());
    withMeshesCheck.setChecked(settings.value(WITH_MESHES, true).toBool());
    withOverlayCheck.setChecked(settings.value(WITH_OVERLAY, true).toBool());
    withScaleCheck.setChecked(settings.value(WITH_SCALE, true).toBool());
    withSkeletonCheck.setChecked(settings.value(WITH_SKELETON, true).toBool());
    withVpPlanes.setChecked(settings.value(WITH_VP_PLANES, false).toBool());
    updateOptionVisibility();

    settings.endGroup();
}
