#ifndef COMMENTSNODECOMMENTSTAB_H
#define COMMENTSNODECOMMENTSTAB_H

#include <QWidget>
#include <QList>
class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;
class CommentsNodeCommentsTab : public QWidget
{
    Q_OBJECT
public:
    explicit CommentsNodeCommentsTab(QWidget *parent = 0);

signals:
    void updateCommentsTableSignal();
public slots:
    void updateCommentsTable();
    void branchPointOnlyChecked(bool on);

protected:
    QCheckBox *branchNodesOnlyCheckbox;
    QLabel *filterLabel;
    QLineEdit *filterField;
    QLabel *editCommentLabel;
    QLineEdit *editCommentField;
    QPushButton *jumpToNodeButton;
    QTableWidget *nodeTable;
    int tableIndex;
    QList<QString> *nodesWithComment;




};

#endif // COMMENTSNODECOMMENTSTAB_H
