#ifndef TOOLSWIDGET_H
#define TOOLSWIDGET_H

#include <QDialog>
#include <QTabWidget>

#include "toolsquicktabwidget.h"
#include "toolstreestabwidget.h"
#include "toolsnodestabwidget.h"

class ToolsWidget : public QDialog
{
    Q_OBJECT
public:
    explicit ToolsWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    void closeEvent(QCloseEvent *event);
    QTabWidget *tabs;

};

#endif // TOOLSWIDGET_H
