#ifndef TOOLSTREESTABWIDGET_H
#define TOOLSTREESTABWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>

class ToolsTreesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreesTabWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    QLabel *activeTreeLabel;
    QSpinBox *activeTreeSpinBox;

    QPushButton *deleteActiveTreeButton;
    QPushButton *newTreeButton;

    QLabel *commentLabel;
    QSpinBox *commentSpinBox;


};

#endif // TOOLSTREESTABWIDGET_H
