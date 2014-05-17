#include "cwidget.h"


CWidget::CWidget(QWidget *parent) :
    QWidget(parent)
{

}

void CWidget::loadPath(const QString &path) {
    QDeclarativeView view;
    view.setSource(QUrl::fromLocalFile(path));

    //connect(&view, SIGNAL())
}
