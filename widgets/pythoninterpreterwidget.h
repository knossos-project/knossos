#ifndef PYTHONINTERPRETERWIDGET_H
#define PYTHONINTERPRETERWIDGET_H

#include <QVBoxLayout>
#include <QDialog>

#include <PythonQt/gui/PythonQtScriptingConsole.h>

class PythonInterpreterWidget : public QDialog
{
    Q_OBJECT
public:
    QVBoxLayout mainLayout;
    explicit PythonInterpreterWidget(QWidget *parent = 0);
    void loadSettings() {}
    void saveSettings() {}
    void startConsole();

signals:
    void visibilityChanged(bool);

private:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
    PythonQtScriptingConsole *console;
};

#endif // PYTHONINTERPRETERWIDGET_H
