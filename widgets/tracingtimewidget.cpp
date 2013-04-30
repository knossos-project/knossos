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

#include "tracingtimewidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include "knossos-global.h"

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
    emit uncheckSignal();
}
