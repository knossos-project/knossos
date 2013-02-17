#ifndef TOOLSQUICKTABWIDGET_H
#define TOOLSQUICKTABWIDGET_H

#include <QWidget>

class QLabel;
class QSpinBox;
class QPushButton;
class QLineEdit;
class ToolsQuickTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsQuickTabWidget(QWidget *parent = 0);
    void loadSettings();
signals:
    
public slots:
    void activeTreeIdChanged(int value);
    void commentChanged(QString comment);
    void searchForChanged(QString comment);
    void findNextButtonClicked();
    void findPreviousButtonClicked();
    void pushBranchNodeButtonClicked();
    void popBranchNodeButtonClicked();

public:
    QLabel *treeCountLabel, *nodeCountLabel;
    QLabel *activeTreeLabel, *activeNodeLabel;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *activeTreeSpinBox, *activeNodeSpinBox;

    QLabel *commentLabel, *searchForLabel;
    QLineEdit *commentField, *searchForField;
    QPushButton *findNextButton, *findPreviousButton;

    QLabel *branchPointLabel, *onStackLabel;
    QPushButton *pushBranchNodeButton, *popBranchNodeButton;
};

#endif // TOOLSQUICKTABWIDGET_H
