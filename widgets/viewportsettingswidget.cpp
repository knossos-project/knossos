#include "viewportsettingswidget.h"

ViewportSettingsWidget::ViewportSettingsWidget(QWidget *parent) :
    QDialog(parent)
{

}

void ViewportSettingsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
