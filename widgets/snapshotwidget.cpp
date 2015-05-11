#include "snapshotwidget.h"

#include <QHBoxLayout>

SnapshotWidget::SnapshotWidget(QWidget *parent) : QDialog(parent) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("Snapshot Tool");

    auto pathLayout = new QHBoxLayout();
    pathLayout->addWidget(&pathEdit);
    pathLayout->addWidget(&browsePathButton);

    auto viewportChoiceLayout = new QVBoxLayout();
    vpXYRadio.setChecked(true);
    viewportChoiceLayout->addWidget(&vpXYRadio);
    viewportChoiceLayout->addWidget(&vpXZRadio);
    viewportChoiceLayout->addWidget(&vpYZRadio);
    viewportChoiceLayout->addWidget(&vp3dRadio);

    auto imageOptionsLayout = new QVBoxLayout();
    withOverlayCheck.setChecked(true);
    withScaleCheck.setChecked(true);
    imageOptionsLayout->addWidget(&withOverlayCheck);
    imageOptionsLayout->addWidget(&withScaleCheck);

    auto settingsLayout = new QHBoxLayout();
    settingsLayout->addLayout(viewportChoiceLayout);
    auto line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameStyle(QFrame::Sunken);
    settingsLayout->addWidget(line);
    settingsLayout->addLayout(imageOptionsLayout);

    mainLayout.addLayout(pathLayout);
    mainLayout.addLayout(settingsLayout);
    mainLayout.addWidget(&snapshotButton);

    QObject::connect(&snapshotButton, QPushButton::clicked, [this]() {
        auto vp = vpXYRadio.isChecked() ? VIEWPORT_XY :
                  vpXZRadio.isChecked() ? VIEWPORT_XZ :
                  vpYZRadio.isChecked() ? VIEWPORT_YZ :
                                          VIEWPORT_SKELETON;
        emit snapshotRequest(pathEdit.text(), vp, withOverlayCheck.isChecked(), withScaleCheck.isChecked());
    });

    setLayout(&mainLayout);
}

