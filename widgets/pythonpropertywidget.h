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

class PythonInterpreterWidget : public QDialog
{
    Q_OBJECT
public:
    explicit PythonInterpreterWidget(QWidget *parent = 0);
protected:
    PythonQtScriptingConsole *console;
};

class PythonPropertyWidget : public QDialog
{
    Q_OBJECT
public:
    explicit PythonPropertyWidget(QWidget *parent = 0);
protected:
    QPushButton *pythonInterpreterButton;
    QPushButton *autoStartFolderButton;
    QPushButton *workingDirectoryButton;
    QLineEdit *autoStartFolderEdit;
    QCheckBox *autoStartTerminalCheckbox;
    QLineEdit *workingDirectoryEdit;
    PythonInterpreterWidget *interpreter;

    void closeEvent(QCloseEvent *e);

public slots:
    void autoStartFolderButtonClicked();
    void openTerminal();
    void closeTerminal();
    void saveSettings();
    void loadSettings();
    void workingDirectoryButtonClicked();



};

#endif // PYTHONPROPERTYWIDGET_H
