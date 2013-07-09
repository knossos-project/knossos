#include "coordinatebarwidget.h"
#include "knossos-global.h"
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>

extern struct stateInfo *state;

CoordinateBarWidget::CoordinateBarWidget(QWidget *parent) :
    QWidget(parent)
{
    copyButton = new QPushButton("Copy");
    pasteButton = new QPushButton("Paste");

    /*
    this->toolBar = new QToolBar();
    this->addToolBar(toolBar);
    this->toolBar->addWidget(copyButton);
    this->toolBar->addWidget(pasteButton);
    */

    xField = new QSpinBox();
    xField->setMaximum(10000);
    xField->setMinimumWidth(75);

    xField->setValue(state->viewerState->currentPosition.x);
    yField = new QSpinBox();
    yField->setMaximum(10000);
    yField->setMinimumWidth(75);
    yField->setValue(state->viewerState->currentPosition.y);
    zField = new QSpinBox();
    zField->setMaximum(10000);
    zField->setMinimumWidth(75);
    zField->setValue(state->viewerState->currentPosition.z);

    xLabel = new QLabel("x");
    yLabel = new QLabel("y");
    zLabel = new QLabel("z");
}

