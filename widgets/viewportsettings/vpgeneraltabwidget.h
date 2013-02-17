#ifndef VPGENERALTABWIDGET_H
#define VPGENERALTABWIDGET_H

#include <QWidget>

class QLabel;
class QCheckBox;
class QSpinBox;
class QRadioButton;
class VPGeneralTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VPGeneralTabWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    void lightEffectsChecked(bool on);
    void hightlightActiveTreeChecked(bool on);
    void showAllNodeIdsChecked(bool on);
    void overrideNodeRadiusChecked(bool on);

    void linesAndPointsChecked(bool on);
    void Skeleton3dChecked(bool on);
protected:
    QLabel *skeletonVisualizationLabel;
    QLabel *skeletonRenderingModelLabel;
    QCheckBox *lightEffectsButton;
    QCheckBox *hightlightActiveTreeButton;
    QCheckBox *showAllNodeIdsButton;
    QCheckBox *overrideNodeRadiusButton;
    QSpinBox *overrideNodeRadiusSpinBox;
    QSpinBox *edgeNodeRadiusRatioSpinBox;
    QLabel *edgeNodeRadiusRatioLabel;

    QRadioButton *linesAndPointsButton;
    QRadioButton *Skeleton3dButton;
};

#endif // VPGENERALTABWIDGET_H
