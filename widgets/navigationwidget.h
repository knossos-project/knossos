#ifndef NAVIGATIONWIDGET_H
#define NAVIGATIONWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpacerItem>
class NavigationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit NavigationWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    void closeEvent(QCloseEvent *event);

    QLabel *generalLabel;
    QLabel *movementSpeedLabel;
    QLabel *jumpFramesLabel;
    QLabel *recenterTimeParallelLabel;
    QLabel *recenterTimeOrthoLabel;

    QSpinBox *movementSpeedSpinBox;
    QSpinBox *jumpFramesSpinBox;
    QSpinBox *recenterTimeParallelSpinBox;
    QSpinBox *recenterTimeOrthoSpinBox;

    QLabel *advanceTracingModesLabel;
    QRadioButton *normalModeButton;
    QRadioButton *additionalViewportDirectionMoveButton;
    QRadioButton *additionalTracingDirectionMoveButton;
    QRadioButton *additionalMirroredMoveButton;

    QLabel *delayTimePerStepLabel;
    QLabel *numberOfStepsLabel;

    QSpinBox *delayTimePerStepSpinBox;
    QSpinBox *numberOfStepsSpinBox;
};

#endif // NAVIGATIONWIDGET_H
