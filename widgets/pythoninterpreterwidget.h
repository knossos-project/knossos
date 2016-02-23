#ifndef PYTHONINTERPRETERWIDGET_H
#define PYTHONINTERPRETERWIDGET_H

#include <QDialog>
#include <QVBoxLayout>

class PythonInterpreterWidget : public QDialog {
    Q_OBJECT
    QVBoxLayout mainLayout;
public:
    explicit PythonInterpreterWidget(QWidget * parent = nullptr);
    void loadSettings() {}
    void saveSettings() {}
    void startConsole();

signals:
    void visibilityChanged(bool);

private:
    void showEvent(QShowEvent * event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent * event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
};

#endif // PYTHONINTERPRETERWIDGET_H
