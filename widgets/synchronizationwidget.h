#ifndef SYNCHRONIZATIONWIDGET_H
#define SYNCHRONIZATIONWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
class SynchronizationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit SynchronizationWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    void closeEvent(QCloseEvent *event);
    QLabel *connectionLabel;
    QLabel *remotePortLabel;
    QSpinBox *remotePortSpinBox;
    QPushButton *connectButton;
};

#endif // SYNCHRONIZATIONWIDGET_H
