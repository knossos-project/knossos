#ifndef MESHESTAB_H
#define MESHESTAB_H

#include <QCheckBox>
#include <QGroupBox>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

class MeshesTab : public QWidget {
    friend class MainWindow;
    Q_OBJECT
    QVBoxLayout mainLayout;
    QFrame separator;
    QGroupBox visibilityGroup{tr("Show meshes in")};
    QVBoxLayout visibilityGroupLayout;
    QCheckBox meshInOrthoVPsCheck{tr("Orthogonal viewports")};
    QCheckBox meshIn3DVPCheck{tr("3D viewport")};
    QCheckBox warnDisabledPickingCheck{tr("Warn on startup if picking is disabled.")};

public:
    explicit MeshesTab(QWidget *parent = nullptr);
    void loadSettings(const QSettings & settings);
    void saveSettings(QSettings & settings) const;
};

#endif // MESHESTAB_H
