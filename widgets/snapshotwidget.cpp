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

SnapshotWidget::SnapshotWidget(QWidget *parent) : QDialog(parent), saveDir(QDir::homePath()) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("Snapshot Tool");
    sizeCombo.addItem("8192 x 8192");
    sizeCombo.addItem("4096 x 4096");
    sizeCombo.addItem("2048 x 2048");
    sizeCombo.addItem("1024 x 1024");
    sizeCombo.setCurrentIndex(0); // 1000 elements default

    auto viewportChoiceLayout = new QVBoxLayout();
    vpXYRadio.setChecked(true);
    viewportChoiceLayout->addWidget(&vpXYRadio);
    viewportChoiceLayout->addWidget(&vpXZRadio);
    viewportChoiceLayout->addWidget(&vpYZRadio);
    viewportChoiceLayout->addWidget(&vp3dRadio);
    QObject::connect(&vp3dRadio, &QRadioButton::toggled, [this](bool checked) {
        withOverlayCheck.setHidden(checked);
        if(Segmentation::singleton().volume_render_toggle == false) {
            withAxesCheck.setVisible(checked);
            withVpPlanes.setVisible(checked);
        }
    });

    auto imageOptionsLayout = new QVBoxLayout();
    withAxesCheck.setChecked(true);
    withOverlayCheck.setChecked(true);
    withScaleCheck.setChecked(true);
    withVpPlanes.setHidden(true);
    imageOptionsLayout->addWidget(&withAxesCheck);
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
                            vpYZRadio.isChecked() ? VIEWPORT_YZ :
                                                    VIEWPORT_SKELETON;
            emit snapshotRequest(path, vp, 8192/pow(2, sizeCombo.currentIndex()), withAxesCheck.isChecked(), withOverlayCheck.isChecked(), withSkeletonCheck.isChecked(), withScaleCheck.isChecked(), withVpPlanes.isChecked());
        }
    });

    setLayout(&mainLayout);
}

ViewportType SnapshotWidget::getCheckedViewport() const {
    return vpXYRadio.isChecked() ? VIEWPORT_XY :
           vpXZRadio.isChecked() ? VIEWPORT_XZ :
           vpYZRadio.isChecked() ? VIEWPORT_YZ :
                                   VIEWPORT_SKELETON;
}

QString SnapshotWidget::defaultFilename() const {
    const QString vp = vpXYRadio.isChecked() ? "XY" :
                   vpXZRadio.isChecked() ? "XZ" :
                   vpYZRadio.isChecked() ? "YZ" :
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
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(VIEWPORT, getCheckedViewport());
    settings.setValue(WITH_AXES, withAxesCheck.isChecked());
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
    setVisible(settings.value(VISIBLE, false).toBool());

    const auto vp = settings.value(VIEWPORT, VIEWPORT_XY).toInt();
    switch((ViewportType)vp) {
        case VIEWPORT_XY: vpXYRadio.setChecked(true); break;
        case VIEWPORT_XZ: vpXZRadio.setChecked(true); break;
        case VIEWPORT_YZ: vpYZRadio.setChecked(true); break;
        default: vp3dRadio.setChecked(true); break;
    }
    withAxesCheck.setChecked(settings.value(WITH_AXES, true).toBool());
    withOverlayCheck.setChecked(settings.value(WITH_OVERLAY, true).toBool());
    withSkeletonCheck.setChecked(settings.value(WITH_SKELETON, true).toBool());
    withScaleCheck.setChecked(settings.value(WITH_SCALE, true).toBool());
    withVpPlanes.setChecked(settings.value(WITH_VP_PLANES, false).toBool());
    saveDir = settings.value(SAVE_DIR, QDir::homePath() + "/").toString();

    settings.endGroup();
}
