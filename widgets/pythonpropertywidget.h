#ifndef PYTHONPROPERTYWIDGET_H
#define PYTHONPROPERTYWIDGET_H

#include <QWidget>
#include <QDialog>

#include <PythonQt/PythonQt.h>
#include <PythonQt/gui/PythonQtScriptingConsole.h>

class QPushButton;
class QLineEdit;
class QLabel;
class QCheckBox;
class PythonPropertyWidget : public QDialog
{
    Q_OBJECT
public:
    explicit PythonPropertyWidget(QWidget *parent = 0);
protected:
    QPushButton *pythonInterpreterButton;
    QPushButton *autoStartFolderButton;
    QPushButton *saveButton;
    QPushButton *defaultPreferences;
    QPushButton *workingDirectoryButton;
    QLineEdit *pythonInterpreterField;
    QLineEdit *autoStartFolder;
    QCheckBox *autoStartTerminal;
    QLineEdit *workingDirectory;
    PythonQtScriptingConsole *console;

    void closeEvent(QCloseEvent *e);

signals:
    void executeUserScriptsSignal();
    void changeWorkingDirectory();
public slots:
    void pythonInterpreterButtonClicked();
    void autoStartFolderButtonClicked();
    void autoConf();
    void openTerminal();
    void closeTerminal();
    void saveSettings();
    void loadSettings();
    void autoStartTerminalClicked(bool on);
    void workingDirectoryButtonClicked();



};

#endif // PYTHONPROPERTYWIDGET_H
