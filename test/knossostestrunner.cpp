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
#include <QPlainTextEdit>

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
    treeWidget->setColumnCount(2);

    QTreeWidgetItem *header = new QTreeWidgetItem();
    header->setText(0, "TestCases");
    treeWidget->setHeaderItem(header);

    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(treeWidget);
    setLayout(mainLayout);

    connect(button, SIGNAL(clicked()), this, SLOT(startTest()));
    pass = fail = total = 0;

    editor = new QPlainTextEdit();
    editor->setFixedHeight(120);

    mainLayout->addWidget(editor);
    connect(treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(testcaseSelected(QTreeWidgetItem*,int)));
}

/**
    Each test case which is a private slot of the class get filtered for the first letter 't'
    Through this convention the test cases of the entire folder can be used.
*/
void KnossosTestRunner::addTestClasses() {

    testCommentsWidget = new TestCommentsWidget();
    testCommentsWidget->reference = reference;

    testDataSavingWidget = new TestDataSavingWidget();
    testDataSavingWidget->reference = reference;

    testNavigationWidget = new TestNavigationWidget();
    testNavigationWidget->viewerReference = reference;

    testOrthogonalViewport = new TestOrthogonalViewport();
    testOrthogonalViewport->reference = reference;

    testSkeletonViewport = new TestSkeletonViewport();
    testSkeletonViewport->reference = reference;

    testToolsWidget = new TestToolsWidget();
    testToolsWidget->reference = reference;

    testZoomAndMultiresWidget = new TestZoomAndMultiresWidget();
    testZoomAndMultiresWidget->reference = reference;

    testclassList = new QList<QObject *>();
    testclassList->append(testCommentsWidget);
    testclassList->append(testDataSavingWidget);
    testclassList->append(testNavigationWidget);
    testclassList->append(testOrthogonalViewport);
    testclassList->append(testSkeletonViewport);
    testclassList->append(testToolsWidget);
    testclassList->append(testZoomAndMultiresWidget);

    const QMetaObject *metaObj;
    for(int a = 0; a < testclassList->size(); a++) {
        metaObj = testclassList->at(a)->metaObject();

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, metaObj->className());
        item->setCheckState(0, Qt::Checked);

        treeWidget->addTopLevelItem(item);

        for(int i = 0; i < metaObj->methodCount(); i++) {
            QMetaMethod method = metaObj->method(i);
            QVariant variant(method.name());
            QString text = variant.toString();
            if(text.startsWith("t")) {
                QTreeWidgetItem *child = new QTreeWidgetItem();
                child->setText(0, text);
                item->addChild(child);
            }
        }
    }
}

void KnossosTestRunner::startTest() {
    QStringList args;
    args << "-silent" << "-o" << "RESULT.xml" << "-xml";

    for(int i = 0; i < testclassList->size(); i++) {
        QTest::qExec(testclassList->at(i), args);
        checkResults();
    }
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

            if(xml.name() == "TestFunction") {
                attributes = xml.attributes();
                attribute = attributes.value("name").toString();

                if(attribute.isEmpty()) {

                } else {
                    qDebug() << attribute;
                    total += 1;
                    currentTestCase = attribute;
                }

            } else if(xml.name() == "Incident") {
                qDebug() << xml.name();
                attributes = xml.attributes();
                attribute = attributes.value("type").toString();

                qDebug() << currentTestCase << " _ ";

                if(attribute == "pass") {
                    pass += 1;
                    QTreeWidgetItem *item = findItem(currentTestCase);
                    if(item) {
                        item->setIcon(0, QIcon(":/images/icons/dialog-ok.png"));
                    }

                } else if(attribute == "fail") {

                    fail += 1;
                    QTreeWidgetItem *item = findItem(currentTestCase);
                    if(item)
                        item->setIcon(0, QIcon(":/images/icons/application-exit.png"));

                    attribute = xml.attributes().value("line").toString();
                    if(attribute.isEmpty()) {

                    } else {
                        qDebug() << attribute;
                        QString message = QString("Line:%1").arg(attribute);
                        item->setText(1, message);
                    }

                    // TODO Message to editor

                }

            } else if(xml.name() == "Message") {
                xml.skipCurrentElement();
            }

        } else {
            xml.readNext();
        }
    }
    passLabel->setText(QString("Pass: %1/%2").arg(pass).arg(total));
    failLabel->setText(QString("Fail: %1").arg(fail));
}

/** This method iterates through the toplevelWidgets and their chilren */
QTreeWidgetItem *KnossosTestRunner::findItem(const QString &name) {
    for(int i = 0; i < treeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem *topLevelWidget = treeWidget->topLevelItem(i);
        for(int k = 0; k < topLevelWidget->childCount(); k++) {
            QTreeWidgetItem *child = topLevelWidget->child(k);
            if(child->text(0) == name) {
                return child;
            }
        }
    }
    return 0;
}

void KnossosTestRunner::testcaseSelected(QTreeWidgetItem *item, int col) {
    editor->clear();
    editor->insertPlainText(item->text(1));
}
