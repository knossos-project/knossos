#include "skeletontab.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>

SkeletonTab::SkeletonTab(QWidget *parent) : QWidget(parent) {
    renderQualitySpin.setMinimum(1);
    renderQualitySpin.setMaximum(20);
    edgeNodeRatioSpin.setSingleStep(0.1);
    depthCutoffSpin.setSingleStep(0.5);
    depthCutoffSpin.setMinimum(0.5);

    auto treeDisplayLayout = new QVBoxLayout();
    treeDisplayLayout->addWidget(&wholeSkeletonRadio);
    treeDisplayLayout->addWidget(&selectedTreesRadio);
    treeDisplayLayout->addWidget(&skeletonInOrthoVPsCheck);
    treeDisplayLayout->addWidget(&skeletonIn3DVPCheck);

    treeSeparator.setFrameShape(QFrame::HLine);
    treeSeparator.setFrameShadow(QFrame::Sunken);
    nodeSeparator.setFrameShape(QFrame::HLine);
    nodeSeparator.setFrameShadow(QFrame::Sunken);

    auto row = 0;
    mainLayout.setAlignment(Qt::AlignTop);
    mainLayout.addWidget(&treeHeaeder, row++, 0);
    mainLayout.addWidget(&treeSeparator, row++, 0, 1, 4);
    mainLayout.addWidget(&highlightActiveTreeCheck, row, 0);  mainLayout.addLayout(treeDisplayLayout, row++, 3, 4, 1);
    mainLayout.addWidget(&highlightIntersectionsCheck, row++, 0);
    mainLayout.addWidget(&lightEffectsCheck, row++, 0);
    mainLayout.addWidget(&ownTreeColorsCheck, row, 0);  mainLayout.addWidget(&loadTreeLUTButton, row++, 1);
    mainLayout.addWidget(&depthCutOffLabel, row, 0);  mainLayout.addWidget(&depthCutoffSpin, row++, 1);
    mainLayout.addWidget(&renderQualityLabel, row, 0);  mainLayout.addWidget(&renderQualitySpin, row++, 1);
    mainLayout.addWidget(&nodeHeaeder, row++, 0);
    mainLayout.addWidget(&nodeSeparator, row++, 0, 1, 4);
    mainLayout.addWidget(&allNodeIDsCheck, row++, 0);
    mainLayout.addWidget(&nodeCommentsCheck, row++, 0);
    mainLayout.addWidget(&overrideNodeRadiusCheck, row, 0); mainLayout.addWidget(&nodeRadiusSpin, row++, 1);
    mainLayout.addWidget(&edgeNodeRatioLabel, row, 0);  mainLayout.addWidget(&edgeNodeRatioSpin, row++, 1);
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
    QObject::connect(&wholeSkeletonRadio, &QRadioButton::toggled, [](const bool checked) {
        if (checked) {
            state->skeletonState->displayMode &= (~DSP_WHOLE & ~DSP_SELECTED_TREES);
            state->skeletonState->displayMode |= DSP_WHOLE;
        }
    });
    QObject::connect(&selectedTreesRadio, &QRadioButton::toggled, [](const bool checked) {
        if (checked) {
            state->skeletonState->displayMode &= (~DSP_WHOLE & ~DSP_SELECTED_TREES);
            state->skeletonState->displayMode |= DSP_SELECTED_TREES;
        }
    });
    QObject::connect(&skeletonInOrthoVPsCheck, &QCheckBox::toggled, [](const bool checked) {
        if (checked) {
            state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
        } else {
            state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
        }
    });
    QObject::connect(&skeletonIn3DVPCheck, &QCheckBox::toggled, [](const bool checked) {
        if (checked) {
            state->skeletonState->displayMode &= ~DSP_SKEL_VP_HIDE;
        } else {
            state->skeletonState->displayMode |= DSP_SKEL_VP_HIDE;
        }
    });
    // nodes
    QObject::connect(&allNodeIDsCheck, &QCheckBox::toggled, [](const bool on) { state->skeletonState->showNodeIDs = on; });
    QObject::connect(&nodeRadiusSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value ) { state->skeletonState->overrideNodeRadiusVal = value; });
    QObject::connect(&nodeCommentsCheck, &QCheckBox::toggled, [](const bool checked) { Viewport::showNodeComments = checked; });
    QObject::connect(&overrideNodeRadiusCheck, &QCheckBox::toggled, [this](const bool on) {
        state->skeletonState->overrideNodeRadiusBool = on;
        nodeRadiusSpin.setEnabled(on);
    });
    QObject::connect(&edgeNodeRatioSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->skeletonState->segRadiusToNodeRadius = value; });
}

void SkeletonTab::loadTreeLUTButtonClicked(QString path) {
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
