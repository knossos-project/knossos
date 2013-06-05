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

#include "commentswidget.h"
#include "commentshortcuts/commentshortcutstab.h"
#include "commentshortcuts/commentspreferencestab.h"
#include "GUIConstants.h"
#include <QSettings>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVariant>
#include <QTableWidget>
#include "knossos-global.h"


extern struct stateInfo *state;

CommentsWidget::CommentsWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Comments Shortcuts");

    this->shortcutTab = new CommentShortCutsTab();
    this->preferencesTab = new CommentsPreferencesTab();

    tabs = new QTabWidget(this);
    tabs->addTab(shortcutTab, "shortcuts");
    tabs->addTab(preferencesTab, "preferences");

}

void CommentsWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(COMMENTS_WIDGET);
    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();
    /*
    this->textFields[0]->setText(settings.value(COMMENT1).toString());
    this->textFields[1]->setText(settings.value(COMMENT2).toString());
    this->textFields[2]->setText(settings.value(COMMENT3).toString());
    this->textFields[3]->setText(settings.value(COMMENT4).toString());
    this->textFields[4]->setText(settings.value(COMMENT1).toString());
    */
    settings.endGroup();

    this->setGeometry(x, y, width, height);

}

void CommentsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(COMMENTS_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());
    /*
    settings.setValue(COMMENT1, this->textFields[0]->text());
    settings.setValue(COMMENT2, this->textFields[1]->text());
    settings.setValue(COMMENT3, this->textFields[2]->text());
    settings.setValue(COMMENT4, this->textFields[3]->text());
    settings.setValue(COMMENT5, this->textFields[4]->text());
    */
    settings.endGroup();
}


void CommentsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit this->uncheckSignal();
}


