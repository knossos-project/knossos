#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "buildinfo.h"

#include <boost/config.hpp>
#include <boost/predef.h>
#include <boost/version.hpp>

#include <gnu/libc-version.h>

AboutDialog::AboutDialog(QWidget * parent) : QDialog(parent), ui(new Ui::AboutDialog) {
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->setupUi(this);
    ui->knossosLabel->setText(tr("KNOSSOS"));
    ui->revisionLabelValue->setText(KREVISION);
    ui->revisionDateLabelValue->setText(KREVISIONDATE);
    ui->qtVersionLabelValue->setText(tr("libc %2\n"
                                        "Qt %1").arg(qVersion(), gnu_get_libc_version()));

    ui->compilerLabelValue->setText(QString{"%1 / C++ %2\n"
                                            "%3\n"
                                            "%5 / %6 %7 / libc %8\n"
                                            "boost %9\n"
                                            "Qt %10"
                                            }.arg(BOOST_COMPILER).arg(BOOST_CXX_VERSION)
                                    .arg(BOOST_STDLIB)
                                    .arg(BOOST_PLATFORM).arg(BOOST_HW_SIMD_X86_NAME)
                                    .arg(QString{"%1.%2.%3"}.arg(BOOST_VERSION_NUMBER_MAJOR(BOOST_HW_SIMD)).arg(BOOST_VERSION_NUMBER_MINOR(BOOST_HW_SIMD)).arg(BOOST_VERSION_NUMBER_PATCH(BOOST_HW_SIMD)))
                                    .arg(""+
                                    #ifdef BOOST_LIB_C_GNU_AVAILABLE
                                        QString{"%1 %2"}.arg(BOOST_LIB_C_GNU_NAME)
                                         .arg(QString{"%1.%2.%3"}.arg(BOOST_VERSION_NUMBER_MAJOR(BOOST_LIB_C_GNU)).arg(BOOST_VERSION_NUMBER_MINOR(BOOST_LIB_C_GNU)).arg(BOOST_VERSION_NUMBER_PATCH(BOOST_LIB_C_GNU)))
                                    #endif
                                    )
                                    .arg(QString{"%1.%2.%3"}.arg(BOOST_VERSION / 100000).arg(BOOST_VERSION / 100 % 1000).arg(BOOST_VERSION % 100))
                                    .arg(QT_VERSION_STR)
                                    );
}

AboutDialog::~AboutDialog() {
    delete ui;
}
