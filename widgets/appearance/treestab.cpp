#include "treestab.h"
#include "widgets/GuiConstants.h"
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
    QObject::connect(&highlightActiveTreeCheck, &QCheckBox::clicked, [](const bool on) { state->viewerState->highlightActiveTree = on; });
    QObject::connect(&highlightIntersectionsCheck, &QCheckBox::clicked, [this](const bool checked) {
        emit showIntersectionsSignal(checked);
        state->viewerState->showIntersections = checked;
    });
    QObject::connect(&lightEffectsCheck, &QCheckBox::clicked, [](const bool on) { state->viewerState->lightOnOff = on; });
    QObject::connect(&ownTreeColorsCheck, &QCheckBox::clicked, [this](const bool checked) {
        if (checked) {//load file if none is cached
            loadTreeLUTButtonClicked(lutFilePath);
        } else {
            state->viewer->loadTreeLUT();
        }
    });
    QObject::connect(&loadTreeLUTButton, &QPushButton::clicked, [this]() { loadTreeLUTButtonClicked(); });
    QObject::connect(&depthCutoffSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->viewerState->depthCutOff = value; });
    QObject::connect(&renderQualitySpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](const int value) { state->viewerState->cumDistRenderThres = value; });
    // tree visibility
    QObject::connect(&wholeSkeletonRadio, &QRadioButton::clicked, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&selectedTreesRadio, &QRadioButton::clicked, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&skeletonInOrthoVPsCheck, &QCheckBox::clicked, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&skeletonIn3DVPCheck, &QCheckBox::clicked, this, &TreesTab::updateTreeDisplay);
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
            state->viewer->loadTreeLUT(path);
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

void TreesTab::saveSettings(QSettings & settings) const {
    settings.setValue(LIGHT_EFFECTS, lightEffectsCheck.isChecked());
    settings.setValue(HIGHLIGHT_ACTIVE_TREE, highlightActiveTreeCheck.isChecked());
    settings.setValue(HIGHLIGHT_INTERSECTIONS, highlightIntersectionsCheck.isChecked());
    settings.setValue(TREE_LUT_FILE_USED, ownTreeColorsCheck.isChecked());
    settings.setValue(TREE_LUT_FILE, lutFilePath);
    settings.setValue(DEPTH_CUTOFF, depthCutoffSpin.value());
    settings.setValue(RENDERING_QUALITY, renderQualitySpin.value());
    settings.setValue(WHOLE_SKELETON, wholeSkeletonRadio.isChecked());
    settings.setValue(ONLY_SELECTED_TREES, selectedTreesRadio.isChecked());
    settings.setValue(SHOW_SKELETON_ORTHOVPS, skeletonInOrthoVPsCheck.isChecked());
    settings.setValue(SHOW_SKELETON_SKELVP, skeletonIn3DVPCheck.isChecked());
}

void TreesTab::loadSettings(const QSettings & settings) {
    lightEffectsCheck.setChecked(settings.value(LIGHT_EFFECTS, true).toBool());
    lightEffectsCheck.clicked(lightEffectsCheck.isChecked());
    highlightActiveTreeCheck.setChecked(settings.value(HIGHLIGHT_ACTIVE_TREE, true).toBool());
    highlightActiveTreeCheck.clicked(highlightActiveTreeCheck.isChecked());
    highlightIntersectionsCheck.setChecked(settings.value(HIGHLIGHT_INTERSECTIONS, false).toBool());
    highlightIntersectionsCheck.clicked(highlightIntersectionsCheck.isChecked());
    depthCutoffSpin.setValue(settings.value(DEPTH_CUTOFF, 5.).toDouble());
    depthCutoffSpin.valueChanged(depthCutoffSpin.value());
    renderQualitySpin.setValue(settings.value(RENDERING_QUALITY, 7).toInt());
    renderQualitySpin.valueChanged(renderQualitySpin.value());
    lutFilePath = settings.value(TREE_LUT_FILE, "").toString();
    //it’s impotant to populate the checkbox after loading the path-string, because emitted signals depend on the lut // TODO VP settings: is that true?
    ownTreeColorsCheck.setChecked(settings.value(TREE_LUT_FILE_USED, false).toBool());
    ownTreeColorsCheck.clicked(ownTreeColorsCheck.isChecked());
    wholeSkeletonRadio.setChecked(settings.value(WHOLE_SKELETON, true).toBool());
    wholeSkeletonRadio.clicked(wholeSkeletonRadio.isChecked());
    selectedTreesRadio.setChecked(settings.value(ONLY_SELECTED_TREES, false).toBool());
    selectedTreesRadio.clicked(selectedTreesRadio.isChecked());
    skeletonInOrthoVPsCheck.setChecked(settings.value(SHOW_SKELETON_ORTHOVPS, true).toBool());
    skeletonInOrthoVPsCheck.clicked(skeletonInOrthoVPsCheck.isChecked());
    skeletonIn3DVPCheck.setChecked(settings.value(SHOW_SKELETON_SKELVP, true).toBool());
    skeletonIn3DVPCheck.clicked(skeletonIn3DVPCheck.isChecked());
}
