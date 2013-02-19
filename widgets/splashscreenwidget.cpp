#include "splashscreenwidget.h"
#include <QPixmap>
#include <QVBoxLayout>
#include <QLabel>

SplashScreenWidget::SplashScreenWidget(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();


    trick = new QLabel();

    trick->setPixmap(QPixmap("splash"));

    mainLayout->addWidget(trick);
    setLayout(mainLayout);

}
