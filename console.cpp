#include "console.h"

Console::Console(QWidget *parent) :
    QWidget(parent)
{
    this->editor = new QPlainTextEdit(this);
    this->editor->setStyleSheet("background : black");
    this->editor->setStyleSheet("color : white");

}
