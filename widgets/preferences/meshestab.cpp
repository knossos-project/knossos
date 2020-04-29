#include "meshestab.h"
#include "widgets/GuiConstants.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QSignalBlocker>

MeshesTab::MeshesTab(QWidget *parent) : QWidget(parent) {
    warnDisabledPickingCheck.setChecked(true);
    warnDisabledPickingCheck.setToolTip(tr("Mesh picking is disabled if your graphics driver does not at least support OpenGL 3.0."));
    visibilityGroupLayout.addWidget(&meshInOrthoVPsCheck);
    visibilityGroupLayout.addWidget(&meshIn3DVPCheck);
    visibilityGroup.setLayout(&visibilityGroupLayout);

    static auto opacityControls = [](auto & label, auto & slider, auto & spin, auto & layout, auto setter){
        slider.setRange(0, 100);
        slider.setOrientation(Qt::Horizontal);
        spin.setRange(0, 1);
        spin.setSingleStep(0.01);
        spin.setPrefix("×");
        layout.addWidget(&label);
        layout.addWidget(&slider);
        layout.addWidget(&spin);
        QObject::connect(&spin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [&slider, setter](const double value) {
            {
                QSignalBlocker blocker{slider};
                slider.setValue(value * 100);
            }
            setter(value);
        });
        QObject::connect(&slider, static_cast<void(QSlider::*)(int)>(&QSlider::valueChanged), [&spin, setter](const int value) {
            {
                QSignalBlocker blocker{spin};
                spin.setValue(value/100.);
            }
            setter(spin.value());
        });
    };
    opacityControls(alphaLabel3d, alphaSlider3d, alphaSpin3d, alphaLayout, [](auto val){state->viewer->setMesh3dAlphaFactor(val);});
    opacityControls(alphaLabelSlicing, alphaSliderSlicing, alphaSpinSlicing, alphaLayout, [](auto val){state->viewer->setMeshSlicingAlphaFactor(val);});

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
    QObject::connect(&meshInOrthoVPsCheck, &QCheckBox::toggled, [](const bool checked) { state->viewerState->meshDisplay.setFlag(TreeDisplay::ShowInOrthoVPs, checked); });
    QObject::connect(&meshIn3DVPCheck, &QCheckBox::toggled, [](const bool checked) { state->viewerState->meshDisplay.setFlag(TreeDisplay::ShowIn3DVP, checked); });
    QObject::connect(state->viewer, &Viewer::mesh3dAlphaFactorChanged, &alphaSpin3d, &QDoubleSpinBox::setValue);
    QObject::connect(state->viewer, &Viewer::meshSlicingAlphaFactorChanged, &alphaSpinSlicing, &QDoubleSpinBox::setValue);
}

void MeshesTab::setMeshVisibility(bool showIn3DVP, bool showInOrthoVPs) {
    meshInOrthoVPsCheck.setChecked(showInOrthoVPs);
    meshIn3DVPCheck.setChecked(showIn3DVP);
}

void MeshesTab::loadSettings(const QSettings & settings) {
    meshInOrthoVPsCheck.setChecked(settings.value(SHOW_MESH_ORTHOVPS, true).toBool());
    meshInOrthoVPsCheck.toggled(meshInOrthoVPsCheck.isChecked());
    meshIn3DVPCheck.setChecked(settings.value(SHOW_MESH_3DVP, true).toBool());
    meshIn3DVPCheck.toggled(meshIn3DVPCheck.isChecked());
    warnDisabledPickingCheck.setChecked(settings.value(WARN_DISABLED_MESH_PICKING, true).toBool());
    alphaSpin3d.setValue(settings.value(MESH_ALPHA_3D, 1).toDouble());
    alphaSpinSlicing.setValue(settings.value(MESH_ALPHA_SLICING, 0.5).toDouble());
}

void MeshesTab::saveSettings(QSettings & settings) const {
    settings.setValue(SHOW_MESH_ORTHOVPS, meshInOrthoVPsCheck.isChecked());
    settings.setValue(SHOW_MESH_3DVP, meshIn3DVPCheck.isChecked());
    settings.setValue(WARN_DISABLED_MESH_PICKING, warnDisabledPickingCheck.isChecked());
    settings.setValue(MESH_ALPHA_3D, alphaSpin3d.value());
    settings.setValue(MESH_ALPHA_SLICING, alphaSpinSlicing.value());
}
