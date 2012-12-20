#ifndef CONSOLE_H
#define CONSOLE_H

#include <QWidget>
#include <QPlainTextEdit>

class Console : public QWidget
{
    Q_OBJECT
public:
    explicit Console(QWidget *parent = 0);
    
signals:
    
public slots:
    
protected:
    QPlainTextEdit *editor;

};

#endif // CONSOLE_H
