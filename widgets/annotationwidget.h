#ifndef ANNOTATIONWIDGET_H
#define ANNOTATIONWIDGET_H

#include <QDialog>
#include <QTabWidget>

#include "tools/toolstreeviewtab.h"
#include "tools/toolscommandstab.h"

class QVBoxLayout;
class QLabel;
class AnnotationWidget : public QDialog
{
    Q_OBJECT
public:
    ToolsTreeviewTab *treeviewTab;
    ToolsCommandsTab *commandsTab;
    QVBoxLayout *mainLayout;
    explicit AnnotationWidget(QWidget *parent = 0);
protected:
    QTabWidget *tabs;
    QLabel *treeCountLabel;
    QLabel *nodeCountLabel;
    QLabel *listedNodesLabel;

signals:
    
public slots:
    void nodesInList(int n);
    void update();
    void saveSettings();
    void loadSettings();
};

#endif // ANNOTATIONWIDGET_H
