#include "property_query.h"

QString PropertyQuery::getComment() const {
    return properties.value("comment").toString();
}

void PropertyQuery::setComment(const QString & comment) {
    properties.insert("comment", comment);
}
