#ifndef VPSKELETONVIEWPORTWIDGET_H
#define VPSKELETONVIEWPORTWIDGET_H

#include <QWidget>

class QLabel;
class QCheckBox;
class QRadioButton;
class VPSkeletonViewportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VPSkeletonViewportWidget(QWidget *parent = 0);
    
signals:
    
public slots:

protected:
    QLabel *datasetVisualizationLabel, *skeletonDisplayModesLabel;
    QCheckBox *showXYPlaneCheckBox, *showXZPlaneCheckBox, *showYZPlaneCheckBox;
    QRadioButton *wholeSkeletonButton, *onlyCurrentCubeButton, *onlyActiveTreeButton, *hideSkeletonButton;
    QLabel *view3dlabel;
    QCheckBox *rotateAroundActiveNodeCheckBox;
    
};

#endif // VPSKELETONVIEWPORTWIDGET_H
