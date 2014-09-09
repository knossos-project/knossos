#ifndef FILE_IO_H
#define FILE_IO_H

#include <QString>

QString annotationFileDefaultName();
QString annotationFileDefaultPath();
void annotationFileLoad(const QString &filename, const QString &treeCmtOnMultiLoad, bool *isSuccess = NULL);
void annotationFileSave(const QString & filename, bool *isSuccess = NULL);
void updateFileName(QString &fileName);

#endif//FILE_IO_H
