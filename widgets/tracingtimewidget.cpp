#include "tracingtimewidget.h"
#include <QLabel>
#include <QVBoxLayout>

TracingTimeWidget::TracingTimeWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Tracing Time");
    this->runningTimeLabel = new QLabel("Running Time: 00:00:00");
    this->tracingTimeLabel = new QLabel("Tracing Time: 00:00:00");
    this->idleTimeLabel = new QLabel("Idle Time: 00:00:00");

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(runningTimeLabel);
    layout->addWidget(tracingTimeLabel);
    layout->addWidget(idleTimeLabel);
    this->setLayout(layout);


}

void TracingTimeWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
