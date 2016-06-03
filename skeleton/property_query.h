#ifndef PROPERTY_QUERY_H
#define PROPERTY_QUERY_H

#include <QString>
#include <QVariantHash>

class PropertyQuery {
public:
    QVariantHash properties;
    QString getComment() const;
    void setComment(const QString & comment);
};

#endif//PROPERTY_QUERY_H
