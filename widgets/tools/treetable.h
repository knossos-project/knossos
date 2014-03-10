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
    void dropEvent(QDropEvent *event) override;
    void focusInEvent(QFocusEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;
signals:
    void deleteTreesSignal();
    void focused(TreeTable *table);
    void updateNodesTable();
public slots:
};

#endif // TREETABLE_H
