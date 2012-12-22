#include "console.h"

Console::Console(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Console");
    this->editor = new QPlainTextEdit(this);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::black);
    palette.setColor(QPalette::Text, Qt::white);
    this->setPalette(palette);

}
