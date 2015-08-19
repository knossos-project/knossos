#ifndef FILE_IO_H
#define FILE_IO_H

#include <QString>

#include <tuple>
#include <vector>

QString annotationFileDefaultName();
QString annotationFileDefaultPath();
void annotationFileLoad(const QString &filename, const QString &treeCmtOnMultiLoad, bool *isSuccess = NULL);
void annotationFileSave(const QString & filename, bool *isSuccess = NULL);
void nmlExport(const QString & filename);
void updateFileName(QString &fileName);
std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> loadLookupTable(const QString & path);

#endif//FILE_IO_H
