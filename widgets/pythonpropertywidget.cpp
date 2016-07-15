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

#include "pythonpropertywidget.h"

#include "GuiConstants.h"
#include "viewer.h"

#include <Python.h>
#include <PythonQt/PythonQt.h>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

PythonPropertyWidget::PythonPropertyWidget(QWidget *parent) :
    QDialog(parent)
{

    setWindowTitle("Python Properties");
    QVBoxLayout *layout = new QVBoxLayout();

    workingDirectoryButton = new QPushButton("Select working directory");
    workingDirectoryEdit = new QLineEdit();
    autoStartFolderEdit = new QLineEdit();
    autoStartFolderEdit->setToolTip("Scripts in this folder were automatically started with KNOSSOS");
    autoStartFolderButton = new QPushButton("Select Autostart Folder");
    autoStartTerminalCheckbox = new QCheckBox("Open Terminal On Start");
    customPathsAppendButton = new QPushButton("Append Custom Path");
    customPathsEdit = new QTextEdit();

    layout->addWidget(workingDirectoryButton);
    layout->addWidget(workingDirectoryEdit);
    layout->addWidget(autoStartFolderButton);
    layout->addWidget(autoStartFolderEdit);
    layout->addWidget(autoStartTerminalCheckbox);
    layout->addWidget(customPathsAppendButton);
    layout->addWidget(customPathsEdit);

    setLayout(layout);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));

    connect(autoStartFolderButton, SIGNAL(clicked()), this, SLOT(autoStartFolderButtonClicked()));
    connect(workingDirectoryButton, SIGNAL(clicked()), this, SLOT(workingDirectoryButtonClicked()));
    connect(customPathsAppendButton, SIGNAL(clicked()), this, SLOT(appendCustomPathButtonClicked()));
}

void PythonPropertyWidget::closeEvent(QCloseEvent *) {
    this->hide();
}

void PythonPropertyWidget::autoStartFolderButtonClicked() {
     state->viewerState->renderInterval = SLOW;
     QString selection = QFileDialog::getExistingDirectory(this, "select the autostart folder", QDir::homePath());
     state->viewerState->renderInterval = FAST;
     if(!selection.isEmpty()) {
         autoStartFolderEdit->setText(selection);
     }
}

void PythonPropertyWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(PYTHON_WORKING_DIRECTORY, workingDirectoryEdit->text());
    settings.setValue(PYTHON_AUTOSTART_FOLDER, autoStartFolderEdit->text());
    settings.setValue(PYTHON_AUTOSTART_TERMINAL, autoStartTerminalCheckbox->isChecked());
    settings.setValue(PYTHON_CUSTOM_PATHS, customPathsEdit->toPlainText().split("\n"));

    settings.endGroup();

}

void PythonPropertyWidget::loadSettings() {
    int width, height, x, y;

    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topRight().x() - this->width() - 20;
        y = QApplication::desktop()->screen()->rect().topRight().y() + 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }

    this->move(x, y);
    this->resize(width, height);

    auto autoStartFolderValue = settings.value(PYTHON_AUTOSTART_FOLDER);
    if(!autoStartFolderValue.isNull() && !autoStartFolderValue.toString().isEmpty()) {
        autoStartFolderEdit->setText(settings.value(PYTHON_AUTOSTART_FOLDER).toString());
    }

    auto workingDirValue = settings.value(PYTHON_WORKING_DIRECTORY);
    if(!workingDirValue.isNull() && !workingDirValue.toString().isEmpty()) {
        workingDirectoryEdit->setText(settings.value(PYTHON_WORKING_DIRECTORY).toString());
    }

    auto autoStartTerminalValue = settings.value(PYTHON_AUTOSTART_TERMINAL);
    if(!autoStartTerminalValue.isNull()) {
        autoStartTerminalCheckbox->setChecked(autoStartTerminalValue.toBool());
    }

    auto customPathsValue = settings.value(PYTHON_CUSTOM_PATHS);
    if(!customPathsValue.isNull()) {
        customPathsEdit->setText(customPathsValue.toStringList().join("\n"));
    }

    settings.endGroup();
}

void PythonPropertyWidget::workingDirectoryButtonClicked() {
     state->viewerState->renderInterval = SLOW;
     QString selection = QFileDialog::getExistingDirectory(this, "select a working directory", QDir::homePath());
     state->viewerState->renderInterval = FAST;
     if(!selection.isEmpty()) {
         workingDirectoryEdit->setText(selection);
     }
}

void PythonPropertyWidget::appendCustomPathButtonClicked() {
     state->viewerState->renderInterval = SLOW;
     QString selection = QFileDialog::getExistingDirectory(this, "select custom path directory", QDir::homePath());
     state->viewerState->renderInterval = FAST;
     if(!selection.isEmpty()) {
         customPathsEdit->append(selection);
     }
}
