#ifndef SYNCHRONIZATIONWIDGET_H
#define SYNCHRONIZATIONWIDGET_H

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
class QSpinBox;
class QPushButton;
class QLabel;
class SynchronizationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit SynchronizationWidget(QWidget *parent = 0);
    QSpinBox *remotePortSpinBox;
    QPushButton *connectButton;
    bool connected;
    void loadSettings();
    void saveSettings();
signals:
    void uncheckSignal();
public slots:
    void connectButtonPushed();
    void updateConnectionInfo();
    void updateDisconnectionInfo();
protected:
    void closeEvent(QCloseEvent *event);
    QLabel *connectionLabel;
    QLabel *remotePortLabel;

};

#endif // SYNCHRONIZATIONWIDGET_H
