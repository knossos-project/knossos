/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include <math.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QSettings>
#include <QSpacerItem>
#include <QGroupBox>
#include <QPalette>
#include <QColor>
#include <math.h>
#include <QIcon>
#include "knossos-global.h"
#include "tracingtimewidget.h"
#include <QTableWidget>
#include <QHeaderView>
extern  stateInfo *state;

TracingTimeWidget::TracingTimeWidget(QWidget *parent) :
    QDialog(parent)
{
    //setAttribute(Qt::WA_TranslucentBackground, true);
    const int LEFT = 0, RIGHT = 1;
    setWindowIcon(QIcon(":/images/icons/appointment.png"));
    this->setWindowTitle("Tracing Time");

    runningLabelItem = new QTableWidgetItem("Running Time", QTableWidgetItem::Type);
    runningLabelItem->setFlags(Qt::NoItemFlags);
    runningTimeItem = new QTableWidgetItem("00:00:00", QTableWidgetItem::Type);
    runningTimeItem->setFlags(Qt::NoItemFlags);
    tracingLabelItem = new QTableWidgetItem("Tracing Time", QTableWidgetItem::Type);
    tracingLabelItem->setFlags(Qt::NoItemFlags);
    tracingTimeItem = new QTableWidgetItem("00:00:00");
    tracingTimeItem->setFlags(Qt::NoItemFlags);
    idleLabelItem = new QTableWidgetItem("Idle Time", QTableWidgetItem::Type);
    idleLabelItem->setFlags(Qt::NoItemFlags);
    idleTimeItem = new QTableWidgetItem("00:00:00");
    idleTimeItem->setFlags(Qt::NoItemFlags);

    QStringList header;
    header << "Category" << "Time";

    QTableWidget *table = new QTableWidget(3, 2);
    table->setMaximumHeight(100);
    table->setStyleSheet("color:black;");
    table->setHorizontalHeaderLabels(header);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setStyleSheet("::section:horizontal{background-color:#A41B11; font-weight:bold; color:white;} color: black;");

    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setMaximumSectionSize(20);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    table->setItem(0, LEFT, runningLabelItem);
    table->setItem(0, RIGHT, runningTimeItem);
    table->setItem(1, LEFT, tracingLabelItem);
    table->setItem(1, RIGHT, tracingTimeItem);
    table->setItem(2, LEFT, idleLabelItem);
    table->setItem(2, RIGHT, idleTimeItem);



    QVBoxLayout *localLayout = new QVBoxLayout();
    localLayout->addWidget(table);

    QGroupBox *groupBox = new QGroupBox();    
    QVBoxLayout *mainLayout = new QVBoxLayout();

    //groupBox->setLayout(localLayout);
    mainLayout->addLayout(localLayout);
    setLayout(mainLayout);

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshTime()));
    timer->start(1000);

   this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void TracingTimeWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}

void TracingTimeWidget::refreshTime() {
    int time = state->time.elapsed();

    int hoursRunningTime = (int)(time * 0.001 / 3600.0);//
    int minutesRunningTime = (int)(time * 0.001/60.0 - hoursRunningTime * 60);
    int secondsRunningTime = (int)(time * 0.001 - hoursRunningTime * 3600 - minutesRunningTime * 60);

    QString forLabel = QString().sprintf("%02d:%02d:%02d", hoursRunningTime, minutesRunningTime, secondsRunningTime);

    this->runningTimeItem->setText(forLabel);

}

void TracingTimeWidget::checkIdleTime() {

    int time = state->time.elapsed();

    state->skeletonState->idleTimeLast = state->skeletonState->idleTimeNow;
    state->skeletonState->idleTimeNow = time;
    if (state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast > 600000) { //tolerance of 10 minutes
        state->skeletonState->idleTime += state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast;
        state->skeletonState->idleTimeSession += state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast;

        state->skeletonState->unsavedChanges = true;

        int hoursIdleTime = (int)(floor(state->skeletonState->idleTimeSession * 0.001) / 3600.0);
        int minutesIdleTime = (int)(floor(state->skeletonState->idleTimeSession * 0.001) / 60.0 - hoursIdleTime * 60);
        int secondsIdleTime = (int)(floor(state->skeletonState->idleTimeSession * 0.001) - hoursIdleTime * 3600 - minutesIdleTime * 60);

        QString idleString = QString().sprintf("%02d:%02d:%02d", hoursIdleTime, minutesIdleTime, secondsIdleTime);
        this->idleTimeItem->setText(idleString);
    }

    int hoursTracingTime = (int)((floor(time *0.001) - floor(state->skeletonState->idleTimeSession *0.001)) / 3600.0);
    int minutesTracingTime = (int)((floor(time *0.001) - floor(state->skeletonState->idleTimeSession *0.001)) /60.0 - hoursTracingTime * 60);
    int secondsTracingTime = (int)((floor(time *0.001) - floor(state->skeletonState->idleTimeSession *0.001)) - hoursTracingTime * 3600 - minutesTracingTime * 60);

    QString tracingString = QString().sprintf("%02d:%02d:%02d", hoursTracingTime, minutesTracingTime, secondsTracingTime);
    this->tracingTimeItem->setText(tracingString);

    state->viewerState->lastIdleTimeCall = QDateTime::currentDateTimeUtc();
    state->viewerState->renderInterval = FAST;
}
