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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "pythonpropertywidget.h"

#include "GuiConstants.h"
#include "stateInfo.h"
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
    DialogVisibilityNotify(PYTHON_PROPERTY_WIDGET, parent)
{

    setWindowTitle("Python Properties");
    QVBoxLayout *layout = new QVBoxLayout();

    workingDirectoryButton = new QPushButton("Select working directory");
    workingDirectoryEdit = new QLineEdit();
    autoStartFolderEdit = new QLineEdit();
    autoStartFolderEdit->setToolTip("Scripts in this folder were automatically started with KNOSSOS");
    autoStartFolderButton = new QPushButton("Select Autostart Folder");
    customPathsAppendButton = new QPushButton("Append Custom Path");
    customPathsEdit = new QTextEdit();

    layout->addWidget(workingDirectoryButton);
    layout->addWidget(workingDirectoryEdit);
    layout->addWidget(autoStartFolderButton);
    layout->addWidget(autoStartFolderEdit);
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
     const QString selection = state->viewer->suspend([this]{
         return QFileDialog::getExistingDirectory(this, "select the autostart folder", QDir::homePath());
     });
     if(!selection.isEmpty()) {
         autoStartFolderEdit->setText(selection);
     }
}

void PythonPropertyWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(PYTHON_WORKING_DIRECTORY, workingDirectoryEdit->text());
    settings.setValue(PYTHON_AUTOSTART_FOLDER, autoStartFolderEdit->text());
    settings.setValue(PYTHON_CUSTOM_PATHS, customPathsEdit->toPlainText().split("\n"));

    settings.endGroup();

}

void PythonPropertyWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    autoStartFolderEdit->setText(settings.value(PYTHON_AUTOSTART_FOLDER, "").toString());
    workingDirectoryEdit->setText(settings.value(PYTHON_WORKING_DIRECTORY, "").toString());
    customPathsEdit->setText(settings.value(PYTHON_CUSTOM_PATHS).toStringList().join("\n"));
    settings.endGroup();
}

void PythonPropertyWidget::workingDirectoryButtonClicked() {
     const QString selection = state->viewer->suspend([this]{
         return QFileDialog::getExistingDirectory(this, "select a working directory", QDir::homePath());
     });
     if(!selection.isEmpty()) {
         workingDirectoryEdit->setText(selection);
     }
}

void PythonPropertyWidget::appendCustomPathButtonClicked() {
     const QString selection = state->viewer->suspend([this]{
         return QFileDialog::getExistingDirectory(this, "select custom path directory", QDir::homePath());
     });
     if(!selection.isEmpty()) {
         customPathsEdit->append(selection);
     }
}
