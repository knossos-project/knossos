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

#include "preferenceswidget.h"

#include "GuiConstants.h"
#include "viewer.h"

#include <QSettings>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>

PreferencesWidget::PreferencesWidget(QWidget *parent) : DialogVisibilityNotify(parent) {
    setWindowIcon(QIcon(":/resources/icons/view-list-icons-symbolic.png"));
    setWindowTitle("Preferences");
    tabs.addTab(&treesTab, "Trees");
    tabs.addTab(&nodesTab, "Nodes");
    tabs.addTab(&datasetAndSegmentationTab, "Dataset && Segmentation");
    tabs.addTab(&viewportTab, "Viewports");
    tabs.addTab(&saveTab, "Save");
    tabs.addTab(&navigationTab, "Navigation");

    mainLayout.addWidget(&tabs);
    mainLayout.setContentsMargins({});
    mainLayout.setSizeConstraint(QLayout::SetFixedSize);
    setLayout(&mainLayout);
    resize(700, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void PreferencesWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(PREFERENCES_WIDGET);

    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    saveTab.loadSettings(settings);
    datasetAndSegmentationTab.loadSettings(); // these settings must be loaded before treesTab settings, because treesTab’s msaa setting needs to back them up and reload them again.
    navigationTab.loadSettings(settings);
    nodesTab.loadSettings(settings);
    treesTab.loadSettings(settings);
    viewportTab.loadSettings(settings);

    tabs.setCurrentIndex(settings.value(VP_TAB_INDEX, 0).toInt());

    settings.endGroup();
}

void PreferencesWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(PREFERENCES_WIDGET);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(VISIBLE, isVisible());

    saveTab.saveSettings(settings);
    datasetAndSegmentationTab.saveSettings();
    navigationTab.saveSettings(settings);
    nodesTab.saveSettings(settings);
    treesTab.saveSettings(settings);
    viewportTab.saveSettings(settings);

    settings.setValue(VP_TAB_INDEX, tabs.currentIndex());

    settings.endGroup();
}
