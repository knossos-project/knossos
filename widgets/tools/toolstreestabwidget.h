#ifndef TOOLSTREESTABWIDGET_H
#define TOOLSTREESTABWIDGET_H

#include <QWidget>

class QLabel;
class QSpinBox;
class QLineEdit;
class QDoubleSpinBox;
class QPushButton;
class ToolsTreesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreesTabWidget(QWidget *parent = 0);
    void loadSettings();
signals:
    
public slots:
    void activeTreeIDChanged(int value);
    void deleteActiveTreeButtonClicked();
    void newTreeButtonClicked();
    void mergeTreesButtonClicked();
    void id1Changed(int value);
    void id2Changed(int value);

protected:
    QLabel *activeTreeLabel;
    QSpinBox *activeTreeSpinBox;

    QPushButton *deleteActiveTreeButton;
    QPushButton *newTreeButton;

    QLabel *commentLabel;
    QLineEdit *commentField;

    QPushButton *mergeTreesButton, *splitByConnectedComponentsButton, *restoreDefaultColorButton;
    QLabel *id1Label, *id2Label;
    QSpinBox *id1SpinBox, *id2SpinBox;

    QLabel *rLabel, *gLabel, *bLabel, *aLabel;
    QDoubleSpinBox *rSpinBox, *gSpinBox, *bSpinBox, *aSpinBox;

};

#endif // TOOLSTREESTABWIDGET_H
