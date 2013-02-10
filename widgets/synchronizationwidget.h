#ifndef SYNCHRONIZATIONWIDGET_H
#define SYNCHRONIZATIONWIDGET_H

#include <QDialog>


class QSpinBox;
class QPushButton;
class QLabel;
class SynchronizationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit SynchronizationWidget(QWidget *parent = 0);
    QSpinBox *remotePortSpinBox;
    QPushButton *connectButton;
    bool connected;
    void loadSettings();
signals:
    
public slots:
    void connectButtonPushed();
protected:
    void closeEvent(QCloseEvent *event);
    QLabel *connectionLabel;
    QLabel *remotePortLabel;



};

#endif // SYNCHRONIZATIONWIDGET_H
