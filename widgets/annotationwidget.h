#ifndef ANNOTATIONWIDGET_H
#define ANNOTATIONWIDGET_H

#include <QDialog>
#include <QShowEvent>
#include <QTabWidget>

#include "tools/segmentationtab.h"
#include "tools/toolstreeviewtab.h"
#include "tools/toolscommandstab.h"

class QVBoxLayout;
class QLabel;
class AnnotationWidget : public QDialog
{
    Q_OBJECT
public:
    ToolsTreeviewTab *treeviewTab;
    SegmentationTab segmentationTab{this};
    ToolsCommandsTab *commandsTab;
    QVBoxLayout *mainLayout;
    explicit AnnotationWidget(QWidget *parent = 0);
protected:
    QTabWidget *tabs;
    QLabel *treeCountLabel;
    QLabel *nodeCountLabel;
    QLabel *listedNodesLabel;
signals:
    void visibilityChanged(bool);
private:
    void showEvent(QShowEvent *) override {
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *) override {
        emit visibilityChanged(false);
    }
public slots:
    void updateLabels();
    void saveSettings();
    void loadSettings();
};

#endif // ANNOTATIONWIDGET_H
