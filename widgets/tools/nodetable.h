#ifndef NODETABLE_H
#define NODETABLE_H

#include <QTableWidget>

class NodeTable : public QTableWidget {
    Q_OBJECT
public:
    enum {
        NODE_ID, NODE_COMMENT, NODE_X, NODE_Y, NODE_Z, NODE_RADIUS, NODE_COLS
    };
    explicit NodeTable(QWidget *parent);

    bool selectionProtection;
    void resizeToFit();
    void setItem(int row, int column, QTableWidgetItem *item);
    void setRow(const int row, const QString & nodeId, const QString & cmt, const QString & x, const QString & y, const QString & z, const QString & radius);
protected:
    void keyPressEvent(QKeyEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
signals:
    void deleteNodesSignal();
    void focused(NodeTable *table);
};

#endif // NODETABLE_H
