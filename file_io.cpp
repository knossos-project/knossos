#include "file_io.h"

#include <ctime>

#include <QStandardPaths>

#include <quazip/quazipfile.h>

#include "knossos-global.h"
extern stateInfo * state;
#include "segmentation.h"
#include "skeletonizer.h"
#include "viewer.h"

QString annotationFileDefaultName() {
    // Generate a default file name based on date and time.
    auto currentTime = time(nullptr);
    auto localTime = localtime(&currentTime);
    return QString("annotation-%1%2%3-%4%5.000.k.zip")
            //value, right aligned padded to width 2, base 10, filled with '0'
            .arg(localTime->tm_mday, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_mon + 1, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_year, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_hour, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_min, 2, 10, QLatin1Char('0'));
}

QString annotationFileDefaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/annotationFiles/" + annotationFileDefaultName();
}

void annotationFileLoad(const QString & filename, const QString & treeCmtOnMultiLoad) {
    QuaZip archive(filename);
    if (archive.open(QuaZip::mdUnzip)) {
        if (archive.setCurrentFile("annotation.xml")) {
            QuaZipFile file(&archive);
            state->viewer->skeletonizer->loadXmlSkeleton(file, treeCmtOnMultiLoad);
        } else {
            qDebug() << filename << "missing nml";
        }
        if (archive.setCurrentFile("mergelist.txt")) {
            QuaZipFile file(&archive);
            Segmentation::singleton().mergelistLoad(file);
        } else {
            qDebug() << filename << "missing mergelist";
        }
    } else {
        qDebug() << "opening" << filename << "for reading failed";
    }
}

void annotationFileSave(const QString & filename) {
    QuaZip archive_write(filename);
    if (archive_write.open(QuaZip::mdCreate)) {
        auto zipCreateFile = [](QuaZipFile & file_write, const QString & name){
            auto fileinfo = QuaZipNewInfo(name);
            //without permissions set, some archive utilities will not grant any on extract
            fileinfo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
            return file_write.open(QIODevice::WriteOnly, fileinfo);
        };
        QuaZipFile file_write(&archive_write);
        const bool open = zipCreateFile(file_write, "annotation.xml");
        if (open) {
            state->viewer->skeletonizer->saveXmlSkeleton(file_write);
        } else {
            qDebug() << filename << "saving nml failed";
        }
        if (Segmentation::singleton().hasObjects()) {
            QuaZipFile file_write(&archive_write);
            const bool open = zipCreateFile(file_write, "mergelist.txt");
            if (open) {
                Segmentation::singleton().mergelistSave(file_write);
            } else {
                qDebug() << filename << "saving mergelist failed";
            }
        }
    } else {
        qDebug() << "opening" << filename << " for writing failed";
    }
}

/** This is a replacement for the old updateFileName
    It decides if a skeleton file has a revision(case 1) or not(case2).
    if case1 the revision substring is extracted, incremented and will be replaced.
    if case2 an initial revision will be inserted.
    This method is actually only needed for the save or save as slots, if incrementFileName is selected
*/
void updateFileName(QString & fileName) {
    QRegExp versionRegEx = QRegExp("(\\.)([0-9]{3})(\\.)");
    if (versionRegEx.indexIn(fileName) != -1) {
        const auto versionIndex = versionRegEx.pos(2);//get second regex aka version without file extension
        const auto incrementedVersion = fileName.midRef(versionIndex, 3).toInt() + 1;//3 chars following the dot
        fileName.replace(versionIndex, 3, QString("%1").arg(incrementedVersion, 3, 10, QChar('0')));//pad with zeroes
    } else {
        QFileInfo info(fileName);
        fileName = info.dir().absolutePath() + "/" + info.baseName() + ".001." + info.completeSuffix();
    }
}
