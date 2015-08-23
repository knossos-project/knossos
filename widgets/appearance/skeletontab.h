#ifndef SKELETONTAB_H
#define SKELETONTAB_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QGridLayout>
#include <QWidget>

class SkeletonTab : public QWidget
{
    friend class AppearanceWidget;
    friend class EventModel;//hotkey 1 in vps – to toggle the skeleton overlay
    Q_OBJECT
    QGridLayout mainLayout;
    // tree render options
    QLabel treeHeaeder{"<strong>Trees</strong>"};
    QFrame treeSeparator;
    QCheckBox highlightActiveTreeCheck{"Highlight active tree"};
    QCheckBox highlightIntersectionsCheck{"Highlight intersections"};
    QCheckBox lightEffectsCheck{"Enable light effects"};
    QCheckBox ownTreeColorsCheck{"Use custom tree colors"};
    QString lutFilePath;
    QPushButton loadTreeLUTButton{"Load …"};
    QLabel depthCutOffLabel{"Depth cutoff:"};
    QSpinBox depthCutoffSpin;
    QLabel renderQualityLabel{"Rendering quality (1 best, 20 fastest):"};
    QSpinBox renderQualitySpin;
    // tree visibility
    QCheckBox skeletonInOrthoVPsCheck{"Show skeleton in Ortho VPs"};
    QCheckBox skeletonIn3DVPCheck{"Show skeleton in 3D VP"};
    QRadioButton wholeSkeletonRadio{"Show whole skeleton"};
    QRadioButton selectedTreesRadio{"Show only selected trees"};
    // node render options
    QLabel nodeHeaeder{"<strong>Nodes</strong>"};
    QFrame nodeSeparator;
    QCheckBox allNodeIDsCheck{"Show all node IDs"};
    QCheckBox nodeCommentsCheck{"Show node comments"};
    QCheckBox overrideNodeRadiusCheck{"Override node radius"};
    QDoubleSpinBox nodeRadiusSpin;
    QLabel edgeNodeRatioLabel{"Edge : Node radius ratio"};
    QDoubleSpinBox edgeNodeRatioSpin;

    void loadTreeLUTButtonClicked(QString path = "");
public:
    explicit SkeletonTab(QWidget *parent = 0);

signals:
    void showIntersectionsSignal(const bool checked);
public slots:
};

#endif // SKELETONTAB_H
