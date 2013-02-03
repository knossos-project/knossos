#include "splashscreenwidget.h"

SplashScreenWidget::SplashScreenWidget(QWidget *parent) :
    QDialog(parent)
{
    image = new QPixmap("/splash");
}
