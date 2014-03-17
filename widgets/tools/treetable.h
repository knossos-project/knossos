#ifndef TREETABLE_H
#define TREETABLE_H

#include <QTableWidget>

class TreeTable : public QTableWidget {
    Q_OBJECT
public:
    enum {
        TREE_ID, TREE_COLOR, TREE_COMMENT, TREE_COLS
    };
    explicit TreeTable(QWidget *parent);
    int droppedOnTreeID;
    bool selectionProtection;
    void setItem(int row, int column, QTableWidgetItem *item);
    void setRow(const int row, const QString & treeId, const QColor & treeColor, const QString & cmt);
protected:
    void dropEvent(QDropEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
signals:
    void deleteTreesSignal();
    void focused(TreeTable *table);
    void nodesUpdateSignal();
};

#endif // TREETABLE_H
