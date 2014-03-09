#ifndef TREETABLE_H
#define TREETABLE_H

#include <QTableWidget>

class TreeTable : public QTableWidget {
    Q_OBJECT
public:
    explicit TreeTable(QWidget *parent);
    int droppedOnTreeID;
    bool selectionProtection;
    void setItem(int row, int column, QTableWidgetItem *item);
protected:
    void keyPressEvent(QKeyEvent *event);
    void dropEvent(QDropEvent *event);
    void focusInEvent(QFocusEvent *);
signals:
    void focused(TreeTable *table);
    void updateTreeview();
    void deleteTreesSignal();
public slots:
};

#endif // TREETABLE_H
