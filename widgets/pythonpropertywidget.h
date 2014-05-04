#ifndef PYTHONPROPERTYWIDGET_H
#define PYTHONPROPERTYWIDGET_H

#include <QWidget>
#include <QDialog>

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
    QLineEdit *pythonInterpreterField;
    QLineEdit *autoStartFolder;
    QCheckBox *autoStartTerminal;

    void closeEvent(QCloseEvent *e);

signals:
    void executeUserScriptsSignal();
public slots:
    void pythonInterpreterButtonClicked();
    void autoStartFolderButtonClicked();
    void autoConf();
    void openTerminal();
    void saveSettings();
    void loadSettings();
    void autoStartTerminalClicked(bool on);


};

#endif // PYTHONPROPERTYWIDGET_H
