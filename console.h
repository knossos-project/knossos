#ifndef CONSOLE_H
#define CONSOLE_H

#include <QDialog>
#include <QPlainTextEdit>

class Console : public QDialog
{
    Q_OBJECT
public:
    explicit Console(QWidget *parent = 0);

signals:
    
public slots:
    
protected:
    QPlainTextEdit *editor;

    void closeEvent(QCloseEvent *event);

};

#endif // CONSOLE_H
