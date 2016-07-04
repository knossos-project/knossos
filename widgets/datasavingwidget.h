/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef DATASAVINGWIDGET_H
#define DATASAVINGWIDGET_H

#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

class DataSavingWidget : public QDialog {
    Q_OBJECT
    QVBoxLayout mainLayout;
    QFormLayout formLayout;
    QCheckBox autosaveCheckbox{"Auto saving (triggered by changes)"};
    QLabel autosaveIntervalLabel{"Saving interval"};
    QSpinBox autosaveIntervalSpinBox;
    QCheckBox autoincrementFileNameButton{"Auto increment file name"};
    QPushButton revealButton{"Reveal default location"};
    QLabel autosaveLocationEdit;
public:
    explicit DataSavingWidget(QWidget * parent = nullptr);
    void loadSettings();
    void saveSettings();
};

#endif // DATASAVINGWIDGET_H
