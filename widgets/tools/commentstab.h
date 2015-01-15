#ifndef COMMENTSTAB_H
#define COMMENTSTAB_H

#include "commentsetting.h"

#include <QAbstractListModel>
#include <QCheckBox>
#include <QShortcut>
#include <QTreeView>
#include <QWidget>

class CommentsModel : public QAbstractListModel {
Q_OBJECT
    friend class CommentsTab;

protected:
    const std::vector<QString> header{"shortcut", "comment", "color", "node radius"};

public:
    void fill();
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
};


class CommentsTab : public QWidget
{
    Q_OBJECT

    QCheckBox useCommentColorsCheckbox{"Use custom comment colors"};
    QCheckBox useCommentRadiusCheckbox{"Use custom comment radius"};
    CommentsModel commentModel;
    QTreeView commentsTable;
public:
    void saveSettings();
    void loadSettings();
    explicit CommentsTab(QWidget *parent = nullptr);
    ~CommentsTab();

signals:

public slots:
};

#endif // COMMENTSTAB_H
