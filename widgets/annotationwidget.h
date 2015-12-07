#ifndef ANNOTATIONWIDGET_H
#define ANNOTATIONWIDGET_H

#include "tools/segmentationtab.h"
#include "tools/toolstreeviewtab.h"
#include "tools/toolscommandstab.h"
#include "tools/commentstab.h"

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
    CommentsTab commentsTab;
    explicit AnnotationWidget(QWidget *parent = 0);
    void setSegmentationVisibility(const bool visible);
    void saveSettings();
    void loadSettings();

signals:
    void visibilityChanged(bool);

private:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
};

#endif // ANNOTATIONWIDGET_H
