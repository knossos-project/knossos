#ifndef FILE_IO_H
#define FILE_IO_H

#include <QString>

QString annotationFileDefaultName();
QString annotationFileDefaultPath();
void annotationFileLoad(const QString &filename, const QString &treeCmtOnMultiLoad);
void annotationFileSave(const QString & filename);
void updateFileName(QString &fileName);

#endif//FILE_IO_H
