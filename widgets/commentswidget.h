#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include <QDialog>


class QLineEdit;
class QLabel;
class QPushButton;
class QEvent;
class CommentsWidget : public QDialog
{
    Q_OBJECT
public:
    explicit CommentsWidget(QWidget *parent = 0);
    QLineEdit **textFields;
    bool eventFilter(QObject *obj, QEvent *event);
    void loadSettings();

signals:
    
public slots:
    void deleteComments();

protected:
    void closeEvent(QCloseEvent *event);
private:
    static const int NUM = 5;
    QLabel **labels;
    QPushButton *button;
};

#endif // COMMENTSWIDGET_H
