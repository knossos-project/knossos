#include "pythonpropertywidget.h"

#include "GuiConstants.h"
#include "scriptengine/scripting.h"
#include "viewer.h"

#include <Python.h>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

PythonInterpreterWidget::PythonInterpreterWidget(QWidget *parent) :
    QDialog(parent), console(NULL)
{
    console = new PythonQtScriptingConsole(parent, PythonQt::self()->getMainModule());
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(console);
    setLayout(layout);
}

PythonPropertyWidget::PythonPropertyWidget(QWidget *parent) :
    QDialog(parent), interpreter(NULL)
{

    setWindowTitle("Python Properties");
    QFormLayout *layout = new QFormLayout();

    workingDirectoryButton = new QPushButton("Select working directory");
    workingDirectoryEdit = new QLineEdit();
    autoStartFolderEdit = new QLineEdit();
    autoStartFolderEdit->setToolTip("Scripts in this folder were automatically started with KNOSSOS");
    autoStartFolderButton = new QPushButton("Select Autostart Folder");
    autoStartTerminalCheckbox = new QCheckBox("Open Terminal On Start");

    layout->addRow(workingDirectoryEdit, workingDirectoryButton);
    layout->addRow(autoStartFolderEdit, autoStartFolderButton);
    layout->addRow(autoStartTerminalCheckbox);

    setLayout(layout);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));

    connect(autoStartFolderButton, SIGNAL(clicked()), this, SLOT(autoStartFolderButtonClicked()));
    connect(workingDirectoryButton, SIGNAL(clicked()), this, SLOT(workingDirectoryButtonClicked()));
}

void PythonPropertyWidget::closeEvent(QCloseEvent *) {
    this->hide();
}

void PythonPropertyWidget::autoStartFolderButtonClicked() {
     state->viewerState->renderInterval = SLOW;
     QString selection = QFileDialog::getExistingDirectory(this, "select the autostart folder", QDir::homePath());
     if(!selection.isEmpty()) {
         autoStartFolderEdit->setText(selection);
     }
     state->viewerState->renderInterval = FAST;
}

void PythonPropertyWidget::openTerminal() {
    if (NULL == interpreter) {
        interpreter = new PythonInterpreterWidget((QWidget*)this->parent());
    }
    interpreter->show();
}

void PythonPropertyWidget::closeTerminal() {
    interpreter->hide();
}

void PythonPropertyWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());

    if(!this->workingDirectoryEdit->text().isEmpty())
        settings.setValue(PYTHON_WORKING_DIRECTORY, this->workingDirectoryEdit->text());
    if(!this->autoStartFolderEdit->text().isEmpty())
        settings.setValue(PYTHON_AUTOSTART_FOLDER, this->autoStartFolderEdit->text());
    settings.setValue(PYTHON_AUTOSTART_TERMINAL, this->autoStartTerminalCheckbox->isChecked());

    settings.endGroup();

}

void PythonPropertyWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() or settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topRight().x() - this->width() - 20;
        y = QApplication::desktop()->screen()->rect().topRight().y() + 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();
    if(visible) {
        show();
    }
    else {
        hide();
    }

    this->move(x, y);
    this->resize(width, height);
    this->setVisible(visible);

    auto autoStartFolderValue = settings.value(PYTHON_AUTOSTART_FOLDER);
    if(!autoStartFolderValue.isNull() and !autoStartFolderValue.toString().isEmpty()) {
        autoStartFolderEdit->setText(settings.value(PYTHON_AUTOSTART_FOLDER).toString());
    }

    auto workingDirValue = settings.value(PYTHON_WORKING_DIRECTORY);
    if(!workingDirValue.isNull() and !workingDirValue.toString().isEmpty()) {
        workingDirectoryEdit->setText(settings.value(PYTHON_WORKING_DIRECTORY).toString());
    }

    auto autoStartTerminalValue = settings.value(PYTHON_AUTOSTART_TERMINAL);
    if(!autoStartTerminalValue.isNull()) {
        autoStartTerminalCheckbox->setChecked(autoStartTerminalValue.toBool());
    }

    settings.endGroup();
}

void PythonPropertyWidget::workingDirectoryButtonClicked() {
    state->viewerState->renderInterval = SLOW;
     QString selection = QFileDialog::getExistingDirectory(this, "select a working directory", QDir::homePath());
     if(!selection.isEmpty()) {
         workingDirectoryEdit->setText(selection);
     }

     state->viewerState->renderInterval = FAST;
}
