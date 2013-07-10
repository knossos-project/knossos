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

#include "console.h"
#include "GUIConstants.h"
#include <QPlainTextEdit>
#include <QSettings>
#include <QDebug>
#include <QMetaType>


Console::Console(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Console");
    this->editor = new QPlainTextEdit(this);
    this->editor->setReadOnly(true);
    this->editor->insertPlainText("Knossos QT\n");
    this->editor->setFont(QFont("Courier", 12, QFont::Normal));

    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::black);
    palette.setColor(QPalette::Text, Qt::white);
    this->setPalette(palette);

    for(int i = 0; i < 20; i++) {
        this->editor->insertPlainText("Foo\n");
    }

}

void Console::resizeEvent(QResizeEvent *event) {
    this->editor->resize(event->size());
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::black);
    palette.setColor(QPalette::Text, Qt::white);
    this->setPalette(palette);
}

void Console::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
}

void Console::loadSettings() {
    int width, height, x, y;
    int visible;

    QSettings settings;
    settings.beginGroup(CONSOLE_WIDGET);
    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();
    settings.endGroup();

}

void Console::saveSettings() {
    QSettings settings;
    settings.beginGroup(CONSOLE_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());
    settings.endGroup();
}

void Console::log(const char *fmt, ...) {
    va_list args;
    char fmtbuffer[1024];

    va_start(args, fmt);
    vsnprintf(fmtbuffer, sizeof fmtbuffer, fmt, args);
    va_end(args);

    editor->appendPlainText(QString(fmtbuffer));






    va_end(args);

}
