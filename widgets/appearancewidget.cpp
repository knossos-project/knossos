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
#include "appearancewidget.h"

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

AppearanceWidget::AppearanceWidget(QWidget *parent) : QDialog(parent) {
    setWindowIcon(QIcon(":/resources/icons/view-list-icons-symbolic.png"));
    setWindowTitle("Appearance Settings");
    tabs.addTab(&treesTab, "Trees");
    tabs.addTab(&nodesTab, "Nodes");
    tabs.addTab(&datasetAndSegmentationTab, "Dataset && Segmentation");
    tabs.addTab(&viewportTab, "Viewports");

    mainLayout.addWidget(&tabs);
    mainLayout.setContentsMargins({});
    setLayout(&mainLayout);

    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void AppearanceWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(APPEARANCE_WIDGET);
    width = settings.value(WIDTH, this->width()).toInt();
    height = settings.value(HEIGHT, this->height()).toInt();
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topRight().x() - this->width() - 20;
        y = QApplication::desktop()->screen()->rect().topRight().y() + this->height();
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = settings.value(VISIBLE, false).toBool();

    datasetAndSegmentationTab.loadSettings(settings);
    nodesTab.loadSettings(settings);
    treesTab.loadSettings(settings);
    viewportTab.loadSettings(settings);

    tabs.setCurrentIndex(settings.value(VP_TAB_INDEX, 0).toInt());

    settings.endGroup();
    setVisible(visible);
    setGeometry(x, y, width, height);
}

void AppearanceWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(APPEARANCE_WIDGET);
    settings.setValue(WIDTH, geometry().width());
    settings.setValue(HEIGHT, geometry().height());
    settings.setValue(POS_X, geometry().x());
    settings.setValue(POS_Y, geometry().y());
    settings.setValue(VISIBLE, isVisible());

    datasetAndSegmentationTab.saveSettings(settings);
    nodesTab.saveSettings(settings);
    treesTab.saveSettings(settings);
    viewportTab.saveSettings(settings);

    settings.setValue(VP_TAB_INDEX, tabs.currentIndex());

    settings.endGroup();
}
