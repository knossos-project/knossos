#ifndef NODETABLE_H
#define NODETABLE_H

#include <QTableWidget>

class NodeTable : public QTableWidget {
    Q_OBJECT
public:
    explicit NodeTable(QWidget *parent);

    bool selectionProtection;
    void setItem(int row, int column, QTableWidgetItem *item);
protected:
    void keyPressEvent(QKeyEvent *event);
    void focusInEvent(QFocusEvent *);
signals:
    void deleteNodesSignal();
    void updateNodesTable();
    void focused(NodeTable *table);
public slots:
};

#endif // NODETABLE_H
