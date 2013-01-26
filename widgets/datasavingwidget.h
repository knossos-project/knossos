#ifndef DATASAVINGWIDGET_H
#define DATASAVINGWIDGET_H

#include <QDialog>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QLabel>
#include "knossos-global.h"

class DataSavingWidget : public QDialog
{
    Q_OBJECT
public:
    explicit DataSavingWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    void autosaveButtonPushed(bool on);
    void autonincrementFileNameButtonPushed(bool on);
protected:
    void closeEvent(QCloseEvent *event);
    QRadioButton *autosaveButton;
    QLabel *autosaveIntervalLabel;
    QSpinBox *autosaveIntervalSpinBox;
    QLabel *autoincrementFileNameLabel;
    QRadioButton *autoincrementFileNameButton;
    
};

#endif // DATASAVINGWIDGET_H
