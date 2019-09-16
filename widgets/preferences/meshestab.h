#ifndef MESHESTAB_H
#define MESHESTAB_H

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QWidget>

class MeshesTab : public QWidget {
    friend class MainWindow;
    Q_OBJECT
    QVBoxLayout mainLayout;
    QFrame separator1;
    QFrame separator2;
    QGroupBox visibilityGroup{tr("Show meshes in")};
    QVBoxLayout visibilityGroupLayout;
    QCheckBox meshInOrthoVPsCheck{tr("Orthogonal viewports")};
    QCheckBox meshIn3DVPCheck{tr("3D viewport")};
    QCheckBox warnDisabledPickingCheck{tr("Warn on startup if picking is disabled.")};
    QHBoxLayout alphaLayout;
    QLabel alphaLabel3d{tr("3D Opacity")};
    QSlider alphaSlider3d;
    QDoubleSpinBox alphaSpin3d;
    QLabel alphaLabelSlicing{tr("Slicing Opacity")};
    QSlider alphaSliderSlicing;
    QDoubleSpinBox alphaSpinSlicing;

public:
    explicit MeshesTab(QWidget *parent = nullptr);
    void setMeshVisibility(bool, bool);
    void loadSettings(const QSettings & settings);
    void saveSettings(QSettings & settings) const;
};

#endif // MESHESTAB_H
