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

#ifndef APPEARANCEWIDGET_H
#define APPEARANCEWIDGET_H

#include "appearance/datasetsegmentationtab.h"
#include "appearance/nodestab.h"
#include "appearance/treestab.h"
#include "appearance/viewporttab.h"

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>

class AppearanceWidget : public QDialog
{
    Q_OBJECT
    QTabWidget tabs;
public:
    explicit AppearanceWidget(QWidget *parent = 0);
    DatasetAndSegmentationTab datasetAndSegmentationTab;
    NodesTab nodesTab;
    TreesTab treesTab;
    ViewportTab viewportTab;
    QVBoxLayout mainLayout;

    void loadSettings();
    void saveSettings();
signals:
    void visibilityChanged(bool);
private:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
};

#endif // APPEARANCEWIDGET_H
