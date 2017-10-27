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

    mainLayout.addWidget(&visibilityGroup);
    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);
    mainLayout.addWidget(&separator);
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
}

void MeshesTab::saveSettings(QSettings & settings) const {
    settings.setValue(SHOW_MESH_ORTHOVPS, meshInOrthoVPsCheck.isChecked());
    settings.setValue(SHOW_MESH_3DVP, meshIn3DVPCheck.isChecked());
    settings.setValue(WARN_DISABLED_MESH_PICKING, warnDisabledPickingCheck.isChecked());
}
