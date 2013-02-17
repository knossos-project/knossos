#ifndef NAVIGATIONWIDGET_H
#define NAVIGATIONWIDGET_H

#include <QDialog>

class QSpinBox;
class QRadioButton;
class QLabel;
class NavigationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit NavigationWidget(QWidget *parent = 0);
    void loadSettings();
    QSpinBox *movementSpeedSpinBox;
    QSpinBox *jumpFramesSpinBox;
    QSpinBox *recenterTimeParallelSpinBox;
    QSpinBox *recenterTimeOrthoSpinBox;

    QRadioButton *normalModeButton;
    QRadioButton *additionalViewportDirectionMoveButton;
    QRadioButton *additionalTracingDirectionMoveButton;
    QRadioButton *additionalMirroredMoveButton;
    QSpinBox *delayTimePerStepSpinBox;
    QSpinBox *numberOfStepsSpinBox;
signals:
    
public slots:
    void movementSpeedChanged(int value);
    void jumpFramesChanged(int value);
    void recenterTimeParallelChanged(int value);
    void recenterTimeOrthoChanged(int value);
    void normalModeButtonClicked(bool on);
    void additionalViewportDirectionMoveButtonClicked(bool on);
    void additionalTracingDirectionMoveButtonClicked(bool on);
    void additionalMirroredMoveButtonClicked(bool on);
    void delayTimePerStepChanged(int value);
    void numberOfStepsChanged(int value);
protected:
    void closeEvent(QCloseEvent *event);

    QLabel *generalLabel;
    QLabel *movementSpeedLabel;
    QLabel *jumpFramesLabel;
    QLabel *recenterTimeParallelLabel;
    QLabel *recenterTimeOrthoLabel;



    QLabel *advanceTracingModesLabel;


    QLabel *delayTimePerStepLabel;
    QLabel *numberOfStepsLabel;


};

#endif // NAVIGATIONWIDGET_H
