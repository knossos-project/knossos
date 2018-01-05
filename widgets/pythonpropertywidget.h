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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef PYTHONPROPERTYWIDGET_H
#define PYTHONPROPERTYWIDGET_H

#include "widgets/DialogVisibilityNotify.h"

#include <QTextEdit>
#include <QWidget>

class QPushButton;
class QLineEdit;
class QLabel;
class QCheckBox;

class PythonPropertyWidget : public DialogVisibilityNotify {
    Q_OBJECT
public:
    explicit PythonPropertyWidget(QWidget *parent = 0);
protected:
    QPushButton *autoStartFolderButton;
    QPushButton *workingDirectoryButton;
    QLineEdit *autoStartFolderEdit;
    QLineEdit *workingDirectoryEdit;
    QPushButton *customPathsAppendButton;
    QTextEdit *customPathsEdit;

    void closeEvent(QCloseEvent *e);
public slots:
    void autoStartFolderButtonClicked();
    void saveSettings();
    void loadSettings();
    void workingDirectoryButtonClicked();
    void appendCustomPathButtonClicked();
};

#endif // PYTHONPROPERTYWIDGET_H
