#include "commentswidget.h"



CommentsWidget::CommentsWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Comments Shortcuts");
    QFormLayout *layout = new QFormLayout();

    labels = new QLabel*[NUM];
    textFields = new QLineEdit*[NUM];
    for(int i = 0; i < NUM; i++) {
        QString tmp;
        tmp = QString("F%1").arg((i+1));
        labels[i] = new QLabel(tmp);

        textFields[i] = new QLineEdit();
        layout->addRow(labels[i], textFields[i]);

    }
    button = new QPushButton("Clear Comments Boxes");
    layout->addWidget(button);
    this->setLayout(layout);

    connect(button, SIGNAL(clicked()), this, SLOT(deleteComments()));
}

void CommentsWidget::deleteComments() {
    for(int i = 0; i < NUM; i++) {
        textFields[i]->clear();
    }
}
