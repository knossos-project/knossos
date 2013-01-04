#ifndef TRACINGTIMEWIDGET_H
#define TRACINGTIMEWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>

class TracingTimeWidget : public QDialog
{
    Q_OBJECT
public:
    explicit TracingTimeWidget(QWidget *parent = 0);
    
signals:
    
public slots:

protected:
    QLabel *runningTimeLabel;
    QLabel *tracingTimeLabel;
    QLabel *idleTimeLabel;

    void closeEvent(QCloseEvent *event);
};

#endif // TRACINGTIMEWIDGET_H
