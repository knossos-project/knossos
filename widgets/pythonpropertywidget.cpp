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

#include "pythonpropertywidget.h"

#include "GuiConstants.h"
#include "stateInfo.h"
#include "viewer.h"
#include "scriptengine/scripting.h"

#include <QCoreApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QProcess>
#include <QSettings>


PythonPropertyWidget::PythonPropertyWidget(QWidget *parent) : DialogVisibilityNotify(PYTHON_PROPERTY_WIDGET, parent) {
    setWindowTitle("Python Properties");

    pluginDirLabel.setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);
    workingDirLabel.setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);
    pluginDirLabel.setToolTip("Scripts in this folder are automatically started with KNOSSOS.");

    int row = 0;
    layout.addWidget(&workingDirLabel, row, 0);
    layout.addWidget(&workingDirButton, row, 1, Qt::AlignRight);
    layout.addWidget(&resetWorkingDirButton, row++, 2, Qt::AlignRight);
    layout.addWidget(&pluginDirLabel, row, 0);
    layout.addWidget(&pluginDirButton, row, 1, Qt::AlignRight);
    layout.addWidget(&resetPluginDirButton, row++, 2, Qt::AlignRight);
    layout.addWidget(&customPathsLabel, row, 0);
    layout.addWidget(&customPathsAppendButton, row++, 2, Qt::AlignLeft);
    layout.addWidget(&customPathsEdit, row, 0, 1, 3);
    setLayout(&layout);

    if (state->scripting != nullptr) {
        QObject::connect(state->scripting, &Scripting::workingDirChanged, [this](const QString & newDir) {
            workingDirLabel.setText(tr("Working directory: %1").arg(newDir));
        });
        QObject::connect(state->scripting, &Scripting::pluginDirChanged, [this](const QString & newDir) {
            pluginDirLabel.setText(tr("Plugin folder: %1").arg(newDir));
        });
    }
    QObject::connect(&resetWorkingDirButton, &QPushButton::clicked, []() {
        state->scripting->changeWorkingDirectory(QCoreApplication::applicationDirPath());
    });
    QObject::connect(&pluginDirButton, &QPushButton::clicked, this, &PythonPropertyWidget::pluginFolderButtonClicked);
    QObject::connect(&resetPluginDirButton, &QPushButton::clicked, []() {
        state->scripting->setPluginDir(state->scripting->getDefaultPluginDir());
    });
    QObject::connect(&workingDirButton, &QPushButton::clicked, this, &PythonPropertyWidget::workingDirectoryButtonClicked);
    QObject::connect(&customPathsAppendButton, &QPushButton::clicked, this, &PythonPropertyWidget::appendCustomPathButtonClicked);
}

void PythonPropertyWidget::pluginFolderButtonClicked() {
     const auto selection = state->viewer->suspend([this]{
         return QFileDialog::getExistingDirectory(this, "select the plugin folder", QDir::homePath());
     });
     if(!selection.isEmpty()) {
         state->scripting->setPluginDir(selection);
     }
}

void PythonPropertyWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    settings.setValue(VISIBLE, isVisible());

    if (state->scripting != nullptr) {
        settings.setValue(PYTHON_WORKING_DIRECTORY, state->scripting->workingDir);
        settings.setValue(PYTHON_PLUGIN_FOLDER, state->scripting->getPluginDir());

        settings.setValue(PYTHON_CUSTOM_PATHS, customPathsEdit.toPlainText().split("\n"));

        settings.beginGroup(PYTHON_REGISTERED_PLUGINS);
        for (auto iter = std::begin(state->scripting->registeredPlugins); iter != std::end(state->scripting->registeredPlugins); iter++) {
            if (QFileInfo{iter.value().first}.exists()) {
                settings.beginGroup(iter.key());
                settings.setValue(PYTHON_PLUGIN_PATH, iter.value().first);
                settings.setValue(PYTHON_PLUGIN_VERSION, iter.value().second);
                settings.endGroup();
            }
        }
        settings.endGroup();
    }
    settings.endGroup();
}

void PythonPropertyWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    if (state->scripting != nullptr) {
        state->scripting->setPluginDir(settings.value(PYTHON_PLUGIN_FOLDER, state->scripting->getDefaultPluginDir()).toString());
        state->scripting->changeWorkingDirectory(settings.value(PYTHON_WORKING_DIRECTORY, "").toString());

        customPathsEdit.setText(settings.value(PYTHON_CUSTOM_PATHS).toStringList().join("\n"));

        settings.beginGroup(PYTHON_REGISTERED_PLUGINS);
        for (auto & key :  settings.childKeys()) {
            settings.beginGroup(key);
            auto absPath = settings.value(PYTHON_PLUGIN_PATH).toString();
            auto version = settings.value(PYTHON_PLUGIN_VERSION).toString();
            state->scripting->registeredPlugins[key] = {absPath, version};
            settings.endGroup();
        }
        settings.endGroup();
    }
    settings.endGroup();

    if (state->scripting != nullptr) {
        state->scripting->initialize();
    }
}

void PythonPropertyWidget::workingDirectoryButtonClicked() {
     const auto selection = state->viewer->suspend([this]{
         return QFileDialog::getExistingDirectory(this, "select a working directory", QDir::homePath());
     });
     if(!selection.isEmpty()) {
         state->scripting->changeWorkingDirectory(selection);
     }
}

void PythonPropertyWidget::appendCustomPathButtonClicked() {
     const auto selection = state->viewer->suspend([this]{
         return QFileDialog::getExistingDirectory(this, "select custom path directory", QDir::homePath());
     });
     if (!selection.isEmpty()) {
        customPathsEdit.append(selection);
     }
}
