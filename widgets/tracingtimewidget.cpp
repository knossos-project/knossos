#include "tracingtimewidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include "knossos-global.h"
#include "mainwindow.h"

extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

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

    loadSettings();
}

void TracingTimeWidget::loadSettings() {

}

void TracingTimeWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    MainWindow *parent = (MainWindow *) this->parentWidget();
    parent->uncheckTracingTimeAction();
}
