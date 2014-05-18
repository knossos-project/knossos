#include "testwidget.h"

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

TestingWidget::TestingWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* l = new QVBoxLayout();
    QLabel* h = new QLabel("Hello World");
    l->addWidget(h);
    setLayout(l);
}