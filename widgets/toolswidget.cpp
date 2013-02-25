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

#include "toolswidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "widgets/tools/toolstreestabwidget.h"
#include "mainwindow.h"

ToolsWidget::ToolsWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Tools");
    tabs = new QTabWidget(this);

    this->toolsQuickTabWidget = new ToolsQuickTabWidget();
    this->toolsNodesTabWidget = new ToolsNodesTabWidget();
    this->toolsTreesTabWidget = new ToolsTreesTabWidget();

    tabs->addTab(toolsQuickTabWidget, "Quick");
    tabs->addTab(toolsTreesTabWidget, "Trees");
    tabs->addTab(toolsNodesTabWidget, "Nodes");

}

void ToolsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    MainWindow *parent = (MainWindow *)this->parentWidget();
    parent->uncheckToolsAction();

}
