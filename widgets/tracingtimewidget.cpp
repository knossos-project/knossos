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

TracingTimeWidget::TracingTimeWidget(QWidget *parent) :
    QDialog(parent)
{
    //setAttribute(Qt::WA_TranslucentBackground, true);
    const int LEFT = 0, RIGHT = 1;
    setWindowIcon(QIcon(":/images/icons/appointment.png"));
    this->setWindowTitle("Tracing Time");

    tracingtimeLabelItem = new QTableWidgetItem("Running Time", QTableWidgetItem::Type);
    tracingtimeLabelItem->setFlags(Qt::NoItemFlags);

    tracingtimeItem = new QTableWidgetItem("00:00", QTableWidgetItem::Type);
    tracingtimeItem->setFlags(Qt::NoItemFlags);

    QStringList header;
    header << "Category" << "Time";

    QTableWidget *table = new QTableWidget(1, 2);
    table->setMaximumHeight(100);
    table->setStyleSheet("color:black;");
    table->setHorizontalHeaderLabels(header);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setStyleSheet("::section:horizontal{font-weight:bold; color:black;}");
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    table->setItem(0, LEFT, tracingtimeLabelItem);
    table->setItem(0, RIGHT, tracingtimeItem);
    QVBoxLayout *localLayout = new QVBoxLayout();
    localLayout->addWidget(table);

    //QGroupBox *groupBox = new QGroupBox();
    QVBoxLayout *mainLayout = new QVBoxLayout();

    //groupBox->setLayout(localLayout);
    mainLayout->addLayout(localLayout);
    setLayout(mainLayout);

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshTime()));
    timer->start(1000);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));

    tracingtimer = new QTimer(this);

    QObject::connect(tracingtimer, &QTimer::timeout, this, &addTracingTime);

    tracingtimer->start(60000);
}

void TracingTimeWidget::refreshTime() {
    int hours = state->skeletonState->tracingTime / 60;
    int minutes = state->skeletonState->tracingTime % 60;
    QString forLabel = QString().sprintf("%02d:%02d", hours, minutes);

    this->tracingtimeItem->setText(forLabel);
}

void TracingTimeWidget::addTracingTime() {

    if(state->skeletonState->traceractive) {
        ++state->skeletonState->tracingTime;
        state->skeletonState->traceractive = false;

        state->time.restart();
    }

    state->viewerState->renderInterval = FAST;
}
