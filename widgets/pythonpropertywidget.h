#ifndef PYTHONPROPERTYWIDGET_H
#define PYTHONPROPERTYWIDGET_H

#include <QDialog>
#include <QTextEdit>
#include <QWidget>

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
    QPushButton *autoStartFolderButton;
    QPushButton *workingDirectoryButton;
    QLineEdit *autoStartFolderEdit;
    QCheckBox *autoStartTerminalCheckbox;
    QLineEdit *workingDirectoryEdit;
    QPushButton *customPathsAppendButton;
    QTextEdit *customPathsEdit;

    void closeEvent(QCloseEvent *e);

public slots:
    void autoStartFolderButtonClicked();
    void saveSettings();
    void loadSettings();
    void workingDirectoryButtonClicked();
    void appendCustomPathButtonClicked();



};

#endif // PYTHONPROPERTYWIDGET_H
