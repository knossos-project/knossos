#ifndef VIEWPORTSETTINGSWIDGET_H
#define VIEWPORTSETTINGSWIDGET_H

#include <QDialog>
#include <QTabWidget>
#include "viewportsettings/vpgeneraltabwidget.h"
#include "viewportsettings/vpsliceplaneviewportwidget.h"
#include "viewportsettings/vpskeletonviewportwidget.h"

class ViewportSettingsWidget : public QDialog
{
    Q_OBJECT
public:
    explicit ViewportSettingsWidget(QWidget *parent = 0);
    
signals:
    
public slots:

protected:
    void closeEvent(QCloseEvent *event);

    QTabWidget *tabs;
    
};

#endif // VIEWPORTSETTINGSWIDGET_H
