#include "knossostestrunner.h"
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QDebug>
#include <QMetaMethod>
#include <QMetaObject>
#include <QTest>
#include <QXmlStreamReader>

KnossosTestRunner::KnossosTestRunner(QWidget *parent) :
    QWidget(parent)
{

    setWindowTitle("Test Runner");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    QPushButton *button = new QPushButton("Go");
    mainLayout->addWidget(button);

    QGroupBox *groupBox = new QGroupBox("Status");

    QVBoxLayout *layout = new QVBoxLayout();

    groupBox->setLayout(layout);
    QHBoxLayout *innerLayout = new QHBoxLayout();
    passLabel = new QLabel("Pass:");
    failLabel = new QLabel("Fail:");
    innerLayout->addWidget(passLabel);
    innerLayout->addWidget(failLabel);
    layout->addLayout(innerLayout);

    treeWidget = new QTreeWidget();
    treeWidget->setColumnCount(1);

    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(treeWidget);
    setLayout(mainLayout);

    connect(button, SIGNAL(clicked()), this, SLOT(startTest()));

    pass = fail = total = 0;
}

void KnossosTestRunner::addTestClasses() {

    testCommentsWidget = new TestCommentsWidget();
    testCommentsWidget->reference = reference;

    const QMetaObject *obj = testCommentsWidget->metaObject();

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, obj->className());
    item->setCheckState(0, Qt::Checked);

    treeWidget->addTopLevelItem(item);

    for(int i = 0; i < obj->methodCount(); i++) {
        QMetaMethod method = obj->method(i);
        QVariant variant(method.name());
        QString text = variant.toString();
        if(text.startsWith("t")) {
            QTreeWidgetItem *child = new QTreeWidgetItem();
            child->setText(0, text);
            item->addChild(child);
        }
    }
}

void KnossosTestRunner::startTest() {
    QStringList args;
    args << "-silent" << "-o" << "RESULT.xml" << "-xml";

    QTest::qExec(testCommentsWidget, args);
    checkResults();
}

void KnossosTestRunner::checkResults() {
    QDir dir = QDir::current();
    QString fileName = ("RESULT.xml");
    QFile file(fileName);

    if(!file.open(QIODevice::ReadOnly)) {
        qErrnoWarning("could not read the specified file");
        return;
    }

    QXmlStreamAttributes attributes;
    QString attribute;
    QString currentTestCase;

    QXmlStreamReader xml(&file);
    while(!xml.atEnd() and !xml.hasError()) {
        if(xml.readNextStartElement()) {
            qDebug() << xml.name();
            if(xml.name() == "TestFunction") {
                attributes = xml.attributes();
                attribute = attributes.value("name").toString();

                if(attribute.isEmpty()) {

                } else {
                    total += 1;
                    currentTestCase = attribute;
                }

            } else if(xml.name() == "Incident") {
                qDebug() << xml.name();
                attributes = xml.attributes();
                attribute = attributes.value("type").toString();
                if(attribute == "pass") {
                    pass += 1;
                    QTreeWidgetItem *item = findItem(currentTestCase);
                    if(item)
                        item->setIcon(0, QIcon(":/images/icons/dialog-ok.png"));

                } else if(attribute == "fail") {
                    fail += 1;
                }
            }

        } else {
            xml.readNext();
        }

    }

    passLabel->setText(QString("Pass: %1/%2").arg(pass).arg(total));
    failLabel->setText(QString("Fail: %1").arg(fail));

}

QTreeWidgetItem *KnossosTestRunner::findItem(const QString &name) {
    for(int i = 0; i < treeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem *topLevelWidget = treeWidget->topLevelItem(i);
        for(int k = 0; k < topLevelWidget->childCount(); k++) {
            QTreeWidgetItem *child = topLevelWidget->child(i);
            if(child->text(0) == name) {
                return child;
            }
        }
    }

    return 0;
}

