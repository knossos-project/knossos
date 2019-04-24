/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#ifndef AUTOSAVETAB_H
#define AUTOSAVETAB_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class QSettings;
class SaveTab : public QWidget {
    Q_OBJECT
    QVBoxLayout mainLayout;

    QGroupBox generalGroup{"General"};
    QVBoxLayout generalLayout;
    QCheckBox autoincrementFileNameButton{"Auto increment filename on every save"};
    QFormLayout locationFormLayout;
    QLabel autosaveLocationLabel;

    QGroupBox autosaveGroup{"Auto saving (triggered by changes)"};
    QFormLayout formLayout;
    QSpinBox autosaveIntervalSpinBox;

    QGroupBox plyGroupBox{tr("Save meshes as…")};
    QHBoxLayout plyLayout;
    QButtonGroup plySaveButtonGroup;
    QRadioButton plySaveAsBinRadio{tr("binary files")};
    QRadioButton plySaveAsTxtRadio{tr("text files")};
    QGroupBox customSaveGroup{"Custom Save"};
    QHBoxLayout customSaveLayout;
    QCheckBox saveTimeButton{"Include annotation time"};
    QCheckBox saveDatasetPathButton{"Include dataset path"};
    QPushButton customSaveButton{"Save as…"};
public:
    explicit SaveTab(QWidget * parent = nullptr);
    void loadSettings(const QSettings &settings);
    void saveSettings(QSettings &settings);
};

#endif//AUTOSAVETAB_H
