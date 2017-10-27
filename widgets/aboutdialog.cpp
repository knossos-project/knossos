#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "buildinfo.h"

AboutDialog::AboutDialog(QWidget * parent) : QDialog(parent), ui(new Ui::AboutDialog) {
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->setupUi(this);
    ui->knossosLabel->setText(tr("KNOSSOS"));
    ui->revisionLabelValue->setText(KREVISION);
    ui->revisionDateLabelValue->setText(KREVISIONDATE);
    ui->qtVersionLabelValue->setText(tr("using %1 (built with %2)").arg(qVersion()).arg(QT_VERSION_STR));

#ifdef __VERSION__
    ui->compilerLabelValue->setText(tr("GCC %1").arg(__VERSION__));
#endif
#ifdef _MSC_FULL_VER
    ui->compilerLabelValue->setText(tr("VC++ cl v%1").arg(_MSC_FULL_VER));
#endif
}

AboutDialog::~AboutDialog() {
    delete ui;
}
