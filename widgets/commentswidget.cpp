#include "commentswidget.h"
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVariant>
#include "knossos-global.h"

extern struct stateInfo *state;

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

        textFields[i]->installEventFilter(this);
        layout->addRow(labels[i], textFields[i]);

    }
    button = new QPushButton("Clear Comments Boxes");
    layout->addWidget(button);
    this->setLayout(layout);

    connect(button, SIGNAL(clicked()), this, SLOT(deleteComments()));

}

void CommentsWidget::loadSettings() {

}

void CommentsWidget::deleteComments() {
    for(int i = 0; i < NUM; i++) {
        textFields[i]->clear();
    }
}

void CommentsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}

/**
  * This method is a replacement for SIGNAL and SLOT. If the user pushs the return button in the respective textField, the method finds out which textField is
  * clicked and sets the corresponding comment in state->viewerState->gui->
  * @todo an optical feedback for the user would be a nice feature
  */
bool CommentsWidget::eventFilter(QObject *obj, QEvent *event) {
    for(int i = 0; i < NUM; i++) {
        if(textFields[i] == obj) {
            if(event->type() == QEvent::KeyPress) {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                if(keyEvent->key() == Qt::Key_Return) {
                    if(!textFields[i]->text().isEmpty()) {
                        if(i == 0) {
                            state->viewerState->gui->comment1 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 1) {
                            state->viewerState->gui->comment2 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 2) {
                            state->viewerState->gui->comment3 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 3) {
                            state->viewerState->gui->comment4 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        } else if(i == 4) {
                            state->viewerState->gui->comment5 = const_cast<char *>(textFields[i]->text().toStdString().c_str());
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}
