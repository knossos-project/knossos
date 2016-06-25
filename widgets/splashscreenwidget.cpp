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

#include "splashscreenwidget.h"

#include "buildinfo.h"
#include "version.h"

#include <QPixmap>

SplashScreenWidget::SplashScreenWidget(QWidget *parent) : QDialog(parent) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    versionLabel.setText(KVERSION);
    revisionLabel.setText(KREVISION);
    revisionDateLabel.setText(KREVISIONDATE);
    seperator.setFrameShape(QFrame::HLine);
    seperator.setFrameShadow(QFrame::Sunken);
#ifdef __VERSION__
    compilerLabel.setText(tr("GCC %1").arg(__VERSION__));
#endif
#ifdef _MSC_FULL_VER
    compilerLabel.setText(tr("VC++ cl v%1").arg(_MSC_FULL_VER));
#endif
    qtLabel.setText(tr("using %1 (built with %2)").arg(qVersion()).arg(QT_VERSION_STR));

    splash.setPixmap(QPixmap(":/resources/splash"));

    mainLayout.addRow(tr("Version"), &versionLabel);
    mainLayout.addRow(tr("Revision"), &revisionLabel);
    mainLayout.addRow(tr("Revision Date"), &revisionDateLabel);
    mainLayout.addWidget(&seperator);
    mainLayout.addRow(tr("Compiler"), &compilerLabel);
    mainLayout.addRow(tr("Qt"), &qtLabel);
    mainLayout.addRow(&splash);
    mainLayout.setSizeConstraint(QLayout::SetFixedSize);
    setLayout(&mainLayout);
}
