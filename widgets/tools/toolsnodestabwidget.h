#ifndef TOOLSNODESTABWIDGET_H
#define TOOLSNODESTABWIDGET_H

#include <QWidget>

class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QCheckBox;
class ToolsNodesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsNodesTabWidget(QWidget *parent = 0);
    void loadSettings();
signals:
    
public slots:
    void activeNodeChanged(int value);
    void idChanged(int value);
    void commentChanged(QString comment);
    void searchForChanged(QString comment);

    void jumpToNodeButtonClicked();
    void deleteNodeButtonClicked();
    void linkNodeWithButtonClicked();
    void findNextButtonClicked();
    void findPreviousButtonClicked();
    void useLastRadiusChecked(bool on);
    void activeNodeRadiusChanged(double value);
    void defaultNodeRadiusChanged(double value);
    void enableCommentLockingChecked(bool on);
    void lockingRadiusChanged(int value);
    void lockToNodesWithCommentChanged(QString comment);
    void lockToActiveNodeButtonClicked();
    void disableLockingButtonClicked();


protected:
    QLabel *activeNodeIdLabel, *idLabel;
    QSpinBox *activeNodeIdSpinBox, *idSpinBox;
    QPushButton *jumpToNodeButton, *deleteNodeButton, *linkNodeWithButton;

    QLabel *commentLabel, *searchForLabel;
    QLineEdit *commentField, *searchForField;
    QPushButton *findNextButton, *findPreviousButton;

    QCheckBox *useLastRadiusBox;
    QLabel *activeNodeRadiusLabel, *defaultNodeRadiusLabel;
    QDoubleSpinBox *activeNodeRadiusSpinBox, *defaultNodeRadiusSpinBox;

    QCheckBox *enableCommentLockingCheckBox;
    QLabel *lockingRadiusLabel, *lockToNodesWithCommentLabel;
    QSpinBox *lockingRadiusSpinBox;
    QLineEdit *lockingToNodesWithCommentField;

    QPushButton *lockToActiveNodeButton;
    QPushButton *disableLockingButton;

};

#endif // TOOLSNODESTABWIDGET_H
