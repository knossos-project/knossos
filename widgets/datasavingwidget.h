#ifndef DATASAVINGWIDGET_H
#define DATASAVINGWIDGET_H

#include <QDialog>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QLabel>

class DataSavingWidget : public QDialog
{
    Q_OBJECT
public:
    explicit DataSavingWidget(QWidget *parent = 0);
    
signals:
    
public slots:

protected:
    void closeEvent(QCloseEvent *event);
    QRadioButton *autosaveButton;
    QLabel *autosaveIntervalLabel;
    QSpinBox *autosaveIntervalSpinBox;
    QLabel *autoincrementFileNameLabel;
    QRadioButton *autoincrementFileNameButton;
    
};

#endif // DATASAVINGWIDGET_H
