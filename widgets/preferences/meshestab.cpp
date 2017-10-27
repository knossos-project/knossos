#include "meshestab.h"
#include "widgets/GuiConstants.h"
#include "stateInfo.h"
#include "viewer.h"

MeshesTab::MeshesTab(QWidget *parent) : QWidget(parent) {
    warnDisabledPickingCheck.setChecked(true);
    warnDisabledPickingCheck.setToolTip(tr("Mesh picking is disabled if your graphics driver does not at least support OpenGL 3.0."));
    visibilityGroupLayout.addWidget(&meshInOrthoVPsCheck);
    visibilityGroupLayout.addWidget(&meshIn3DVPCheck);
    visibilityGroup.setLayout(&visibilityGroupLayout);

    alphaSpin.setRange(0, 1);
    alphaSpin.setSingleStep(0.01);
    alphaSpin.setPrefix("×");
    alphaSlider.setRange(0, 100);
    alphaSlider.setOrientation(Qt::Horizontal);
    alphaLayout.addWidget(&alphaLabel);
    alphaLayout.addWidget(&alphaSlider);
    alphaLayout.addWidget(&alphaSpin);
    QObject::connect(&alphaSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) {
        alphaSlider.setValue(value * 100);
        state->viewerState->meshAlphaFactor = value;
    });
    QObject::connect(&alphaSlider, static_cast<void(QSlider::*)(int)>(&QSlider::valueChanged), [this](const int value) {
        alphaSpin.setValue(value/100.);
        state->viewerState->meshAlphaFactor = alphaSpin.value();
    });

    mainLayout.addWidget(&visibilityGroup);
    for (auto * sep : {&separator1, &separator2}) {
        sep->setFrameShape(QFrame::HLine);
        sep->setFrameShadow(QFrame::Sunken);
    }
    mainLayout.addWidget(&separator1);
    mainLayout.addLayout(&alphaLayout);
    mainLayout.addWidget(&separator2);
    mainLayout.addWidget(&warnDisabledPickingCheck);
    mainLayout.setAlignment(Qt::AlignTop);
    setLayout(&mainLayout);
    QObject::connect(&meshInOrthoVPsCheck, &QCheckBox::clicked, [](const bool checked) { state->viewerState->meshDisplay.setFlag(TreeDisplay::ShowInOrthoVPs, checked); });
    QObject::connect(&meshIn3DVPCheck, &QCheckBox::clicked, [](const bool checked) { state->viewerState->meshDisplay.setFlag(TreeDisplay::ShowIn3DVP, checked); });
}

void MeshesTab::loadSettings(const QSettings & settings) {
    meshInOrthoVPsCheck.setChecked(settings.value(SHOW_MESH_ORTHOVPS, true).toBool());
    meshInOrthoVPsCheck.clicked(meshInOrthoVPsCheck.isChecked());
    meshIn3DVPCheck.setChecked(settings.value(SHOW_MESH_3DVP, true).toBool());
    meshIn3DVPCheck.clicked(meshIn3DVPCheck.isChecked());
    warnDisabledPickingCheck.setChecked(settings.value(WARN_DISABLED_MESH_PICKING, true).toBool());
    alphaSpin.setValue(settings.value(MESH_ALPHA, 1).toDouble());
}

void MeshesTab::saveSettings(QSettings & settings) const {
    settings.setValue(SHOW_MESH_ORTHOVPS, meshInOrthoVPsCheck.isChecked());
    settings.setValue(SHOW_MESH_3DVP, meshIn3DVPCheck.isChecked());
    settings.setValue(WARN_DISABLED_MESH_PICKING, warnDisabledPickingCheck.isChecked());
    settings.setValue(MESH_ALPHA, alphaSpin.value());
}
