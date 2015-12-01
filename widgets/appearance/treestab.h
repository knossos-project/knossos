#ifndef TREESTAB_H
#define TREESTAB_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QWidget>

class TreesTab : public QWidget
{
    friend class AppearanceWidget;
    friend class ViewportBase;//hotkey 1 in vps – to toggle the skeleton overlay
    Q_OBJECT
    QGridLayout mainLayout;
    // tree render options
    QCheckBox highlightActiveTreeCheck{tr("Highlight active tree")};
    QCheckBox highlightIntersectionsCheck{tr("Highlight intersections")};
    QCheckBox lightEffectsCheck{tr("Enable light effects")};
    QCheckBox ownTreeColorsCheck{tr("Use custom tree colors")};
    QString lutFilePath;
    QPushButton loadTreeLUTButton{tr("Load …")};
    QLabel depthCutOffLabel{tr("Depth cutoff:")};
    QDoubleSpinBox depthCutoffSpin;
    QLabel renderQualityLabel{tr("Skeleton rendering quality:")};
    QComboBox renderQualityCombo;
    // tree visibility
    QCheckBox skeletonInOrthoVPsCheck{tr("Show skeleton in Ortho VPs")};
    QCheckBox skeletonIn3DVPCheck{tr("Show skeleton in 3D VP")};
    QRadioButton wholeSkeletonRadio{tr("Show whole skeleton")};
    QRadioButton selectedTreesRadio{tr("Show only selected trees")};

    void updateTreeDisplay();
    void loadTreeLUTButtonClicked(QString path = "");
    void saveSettings(QSettings & settings) const;
    void loadSettings(const QSettings & settings);
public:
    explicit TreesTab(QWidget *parent = 0);

signals:

public slots:
};

#endif // TREESTAB_H
