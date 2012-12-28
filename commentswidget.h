#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>

class CommentsWidget : public QDialog
{
    Q_OBJECT
public:
    explicit CommentsWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    void deleteComments();
private:
    static const int NUM = 5;
    QLabel **labels;
    QLineEdit **textFields;
    QPushButton *button;
};

#endif // COMMENTSWIDGET_H
