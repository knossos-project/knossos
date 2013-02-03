#ifndef SPLASHSCREENWIDGET_H
#define SPLASHSCREENWIDGET_H

#include <QDialog>

class SplashScreenWidget : public QDialog
{
    Q_OBJECT
public:
    explicit SplashScreenWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    QPixmap *image;
};

#endif // SPLASHSCREENWIDGET_H
