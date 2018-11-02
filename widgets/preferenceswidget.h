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
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef PREFERENCESWIDGET_H
#define PREFERENCESWIDGET_H

#include "preferences/savetab.h"
#include "preferences/datasetsegmentationtab.h"
#include "preferences/navigationtab.h"
#include "preferences/meshestab.h"
#include "preferences/nodestab.h"
#include "preferences/treestab.h"
#include "preferences/viewporttab.h"
#include "widgets/DialogVisibilityNotify.h"

#include <QTabWidget>
#include <QVBoxLayout>

class PreferencesWidget : public DialogVisibilityNotify {
    Q_OBJECT
public:
    QTabWidget tabs;
    explicit PreferencesWidget(QWidget *parent = 0);
    DatasetAndSegmentationTab datasetAndSegmentationTab;
    MeshesTab meshesTab;
    NodesTab nodesTab;
    TreesTab treesTab;
    ViewportTab viewportTab;
    SaveTab saveTab;
    NavigationTab navigationTab;
    QVBoxLayout mainLayout;

    void loadSettings();
    void saveSettings();
};

#endif // PREFERENCESWIDGET_H
