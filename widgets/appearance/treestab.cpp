#include "treestab.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>

TreesTab::TreesTab(QWidget *parent) : QWidget(parent) {
    renderQualitySpin.setMinimum(1);
    renderQualitySpin.setMaximum(20);
    depthCutoffSpin.setSingleStep(0.5);
    depthCutoffSpin.setMinimum(0.5);

    auto treeDisplayLayout = new QVBoxLayout();
    treeDisplayLayout->addWidget(&wholeSkeletonRadio);
    treeDisplayLayout->addWidget(&selectedTreesRadio);
    treeDisplayLayout->addWidget(&skeletonInOrthoVPsCheck);
    treeDisplayLayout->addWidget(&skeletonIn3DVPCheck);

    auto row = 0;
    // trees
    mainLayout.setAlignment(Qt::AlignTop);
    mainLayout.addWidget(&highlightActiveTreeCheck, row, 0);  mainLayout.addLayout(treeDisplayLayout, row++, 3, 4, 1);
    mainLayout.addWidget(&highlightIntersectionsCheck, row++, 0);
    mainLayout.addWidget(&lightEffectsCheck, row++, 0);
    mainLayout.addWidget(&ownTreeColorsCheck, row, 0);  mainLayout.addWidget(&loadTreeLUTButton, row++, 1);
    mainLayout.addWidget(&depthCutOffLabel, row, 0);  mainLayout.addWidget(&depthCutoffSpin, row++, 1);
    mainLayout.addWidget(&renderQualityLabel, row, 0);  mainLayout.addWidget(&renderQualitySpin, row++, 1);
    setLayout(&mainLayout);

    // trees render options
    QObject::connect(&highlightActiveTreeCheck, &QCheckBox::toggled, [](const bool on) { state->skeletonState->highlightActiveTree = on; });
    QObject::connect(&highlightIntersectionsCheck, &QCheckBox::toggled, [this](const bool checked) {
        emit showIntersectionsSignal(checked);
        state->skeletonState->showIntersections = checked;
    });
    QObject::connect(&lightEffectsCheck, &QCheckBox::toggled, [](const bool on) { state->viewerState->lightOnOff = on; });
    QObject::connect(&ownTreeColorsCheck, &QCheckBox::toggled, [this](const bool checked) {
        if (checked) {//load file if none is cached
            loadTreeLUTButtonClicked(lutFilePath);
        } else {
            Skeletonizer::singleton().loadTreeLUT();
        }
    });
    QObject::connect(&loadTreeLUTButton, &QPushButton::clicked, [this]() { loadTreeLUTButtonClicked(); });
    QObject::connect(&depthCutoffSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->viewerState->depthCutOff = value; });
    QObject::connect(&renderQualitySpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](const int value) { state->viewerState->cumDistRenderThres = value; });
    // tree visibility
    QObject::connect(&wholeSkeletonRadio, &QRadioButton::toggled, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&selectedTreesRadio, &QRadioButton::toggled, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&skeletonInOrthoVPsCheck, &QCheckBox::toggled, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&skeletonIn3DVPCheck, &QCheckBox::toggled, this, &TreesTab::updateTreeDisplay);
}

void TreesTab::updateTreeDisplay() {
    state->viewerState->skeletonDisplay = 0x0;
    if (selectedTreesRadio.isChecked()) {
        state->viewerState->skeletonDisplay |= SkeletonDisplay::OnlySelected;
    }
    if (skeletonIn3DVPCheck.isChecked()) {
        state->viewerState->skeletonDisplay |= SkeletonDisplay::ShowIn3DVP;
    }
    if (skeletonInOrthoVPsCheck.isChecked()) {
        state->viewerState->skeletonDisplay |= SkeletonDisplay::ShowInOrthoVPs;
    }
}

void TreesTab::loadTreeLUTButtonClicked(QString path) {
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Load Tree Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            Skeletonizer::singleton().loadTreeLUT(path);
            lutFilePath = path;
            ownTreeColorsCheck.setChecked(true);
        }  catch (...) {
            QMessageBox lutErrorBox(QMessageBox::Warning, "LUT loading failed", "LUTs are restricted to 256 RGB tuples", QMessageBox::Ok, this);
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
            ownTreeColorsCheck.setChecked(false);
        }
    } else {
        ownTreeColorsCheck.setChecked(false);
    }
}
