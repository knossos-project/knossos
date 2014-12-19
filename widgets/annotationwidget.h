#ifndef ANNOTATIONWIDGET_H
#define ANNOTATIONWIDGET_H

#include "tools/segmentationtab.h"
#include "tools/toolstreeviewtab.h"
#include "tools/toolscommandstab.h"

#include <QDialog>
#include <QShowEvent>
#include <QTabWidget>

class AnnotationWidget : public QDialog
{
    Q_OBJECT
public:
    QTabWidget tabs;
    QVBoxLayout mainLayout;
    ToolsTreeviewTab treeviewTab;
    SegmentationTab segmentationTab{this};
    ToolsCommandsTab commandsTab;
    explicit AnnotationWidget(QWidget *parent = 0);
    void saveSettings();
    void loadSettings();

signals:
    void visibilityChanged(bool);

private:
    void showEvent(QShowEvent *) override {
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *) override {
        emit visibilityChanged(false);
    }
};

#endif // ANNOTATIONWIDGET_H
