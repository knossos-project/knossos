#ifndef TRACINGTIMEWIDGET_H
#define TRACINGTIMEWIDGET_H

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

#include <QDialog>

class QLabel;
class TracingTimeWidget : public QDialog
{
    Q_OBJECT
public:
    explicit TracingTimeWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();
signals:
    void uncheckSignal();
public slots:
    void refreshTime();
    void checkIdleTime();
protected:
    QLabel *runningTimeLabel;
    QLabel *tracingTimeLabel;
    QLabel *idleTimeLabel;
    QTimer *timer;
    void closeEvent(QCloseEvent *event);
};



#endif // TRACINGTIMEWIDGET_H
