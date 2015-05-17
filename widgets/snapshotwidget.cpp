#include "snapshotwidget.h"

#include "GuiConstants.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QSettings>

SnapshotWidget::SnapshotWidget(QWidget *parent) : QDialog(parent), saveDir(QDir::homePath()) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("Snapshot Tool");

    auto viewportChoiceLayout = new QVBoxLayout();
    vpXYRadio.setChecked(true);
    viewportChoiceLayout->addWidget(&vpXYRadio);
    viewportChoiceLayout->addWidget(&vpXZRadio);
    viewportChoiceLayout->addWidget(&vpYZRadio);
    viewportChoiceLayout->addWidget(&vp3dRadio);
    QObject::connect(&vp3dRadio, &QRadioButton::toggled, [this](bool checked) {
        withOverlayCheck.setDisabled(checked);
        withScaleCheck.setDisabled(checked);
    });

    auto imageOptionsLayout = new QVBoxLayout();
    withOverlayCheck.setChecked(true);
    withScaleCheck.setChecked(true);
    imageOptionsLayout->addWidget(&withOverlayCheck);
    imageOptionsLayout->addWidget(&withSkeletonCheck);
    imageOptionsLayout->addWidget(&withScaleCheck);

    auto settingsLayout = new QHBoxLayout();
    settingsLayout->addLayout(viewportChoiceLayout);
    auto line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameStyle(QFrame::Sunken);
    settingsLayout->addWidget(line);
    settingsLayout->addLayout(imageOptionsLayout);

    mainLayout.addLayout(settingsLayout);
    mainLayout.addWidget(&snapshotButton);

    QObject::connect(&snapshotButton, &QPushButton::clicked, [this]() {
        const auto path = QFileDialog::getSaveFileName(this, tr("Save path"), saveDir + defaultFilename(), tr("Images (*.png *.xpm *.xbm *.jpg *.bmp)"));
        if(path.isEmpty() == false) {
            QFileInfo info(path);
            saveDir = info.absolutePath() + "/";
            const auto vp = vpXYRadio.isChecked() ? VIEWPORT_XY :
                            vpXZRadio.isChecked() ? VIEWPORT_XZ :
                            vpYZRadio.isChecked() ? VIEWPORT_YZ :
                                                    VIEWPORT_SKELETON;
            emit snapshotRequest(path, vp, withOverlayCheck.isChecked(), withSkeletonCheck.isChecked(), withScaleCheck.isChecked());
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
    const QString name = vpXYRadio.isChecked() ? "XY_" :
                   vpXZRadio.isChecked() ? "XZvp_" :
                   vpYZRadio.isChecked() ? "YZvp_" :
                                           "3Dvp_";
    auto pos = &state->viewerState->currentPosition;
    return QString("%0_%1_%2_%3_%4.png").arg(name).arg(state->name).arg(pos->x).arg(pos->y).arg(pos->z);
}

void SnapshotWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(SNAPSHOT_WIDGET);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(VIEWPORT, getCheckedViewport());
    settings.setValue(WITH_OVERLAY, withOverlayCheck.isChecked());
    settings.setValue(WITH_SKELETON, withSkeletonCheck.isChecked());
    settings.setValue(WITH_SCALE, withScaleCheck.isChecked());
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

    withOverlayCheck.setChecked(settings.value(WITH_OVERLAY, true).toBool());
    withSkeletonCheck.setChecked(settings.value(WITH_SKELETON, true).toBool());
    withScaleCheck.setChecked(settings.value(WITH_SCALE, true).toBool());
    saveDir = settings.value(SAVE_DIR, QDir::homePath() + "/").toString();

    settings.endGroup();
}
