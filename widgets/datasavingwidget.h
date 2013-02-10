#ifndef DATASAVINGWIDGET_H
#define DATASAVINGWIDGET_H

#include <QDialog>


class QCheckBox;
class QLabel;
class QSpinBox;
class DataSavingWidget : public QDialog
{
    Q_OBJECT
public:
    explicit DataSavingWidget(QWidget *parent = 0);
    QCheckBox *autosaveButton;
    QLabel *autosaveIntervalLabel;
    QSpinBox *autosaveIntervalSpinBox;
    QLabel *autoincrementFileNameLabel;
    QCheckBox *autoincrementFileNameButton;
    void loadSettings();
signals:
    
public slots:
    void autosaveButtonPushed(bool on);
    void autonincrementFileNameButtonPushed(bool on);
protected:
    void closeEvent(QCloseEvent *event);

    
};

#endif // DATASAVINGWIDGET_H
