// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy,
// distribute, and modify this file as you see fit.
// Authored in 2015 by Dimitri Diakopoulos (http://www.dimitridiakopoulos.com)
// https://github.com/ddiakopoulos/tinyply

#pragma once

#include <QDataStream>
#include <QTextStream>
#include <QVector>

#include <algorithm>
#include <boost/optional/optional.hpp>
#include <functional>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <vector>

namespace tinyply {
    struct DataCursor {
        void * vector;
        std::uint8_t * data;
        std::size_t offset;
        bool realloc{false};
    };

    class PlyProperty {
        void parse_internal(QTextStream & is);
    public:

        enum class Type : std::uint8_t {
            INVALID,
            INT8,
            UINT8,
            INT16,
            UINT16,
            INT32,
            UINT32,
            FLOAT32,
            FLOAT64
        };

        PlyProperty(QTextStream & is);
        PlyProperty(Type type, const QString & name) : propertyType(type), isList(false), name(name) {}
        PlyProperty(Type list_type, Type prop_type, const QString & name, unsigned int listCount) : listType(list_type), propertyType(prop_type), isList(true), listCount(listCount), name(name) {}

        Type listType, propertyType;
        bool isList;
        unsigned int listCount{0};
        QString name;
    };

    inline QString make_key(const QString & a, const QString & b) {
        return (a + "-" + b);
    }

    template<typename T>
    void ply_cast(void * dest, const char * src) {
        *static_cast<T *>(dest) = *reinterpret_cast<const T *>(src);
    }

    template<typename T>
    void ply_cast_float(void * dest, const char * src) {
        *static_cast<T *>(dest) = *reinterpret_cast<const T *>(src);
    }

    template<typename T>
    void ply_cast_double(void * dest, const char * src) {
        *static_cast<T *>(dest) = *reinterpret_cast<const T *>(src);
    }

    template<typename T>
    T ply_read_ascii(QTextStream & is) {
        T data;
        is >> data;
        if (is.status() != QTextStream::Ok) {
            throw std::invalid_argument("Failed to read property because of malformed ply file: stream status " + std::to_string(is.status()));
        }
        return data;
    }

    template<typename T>
    void ply_cast_ascii(void * dest, QTextStream & is) {
        *(static_cast<T *>(dest)) = ply_read_ascii<T>(is);
    }

    struct PropertyInfo { std::size_t stride; QString str; };
    static std::unordered_map<PlyProperty::Type, PropertyInfo> PropertyTable {
        { PlyProperty::Type::INT8,{ 1, "int8" } },
        { PlyProperty::Type::UINT8,{ 1, "uint8" } },
        { PlyProperty::Type::INT16,{ 2, "short" } },
        { PlyProperty::Type::UINT16,{ 2, "ushort" } },
        { PlyProperty::Type::INT32,{ 4, "int" } },
        { PlyProperty::Type::UINT32,{ 4, "uint" } },
        { PlyProperty::Type::FLOAT32,{ 4, "float" } },
        { PlyProperty::Type::FLOAT64,{ 8, "double" } },
        { PlyProperty::Type::INVALID,{ 0, "INVALID" } }
    };

    inline PlyProperty::Type property_type_from_string(const QString & t) {
        if (t == "int8" || t == "char")             return PlyProperty::Type::INT8;
        else if (t == "uint8" || t == "uchar")      return PlyProperty::Type::UINT8;
        else if (t == "int16" || t == "short")      return PlyProperty::Type::INT16;
        else if (t == "uint16" || t == "ushort")    return PlyProperty::Type::UINT16;
        else if (t == "int32" || t == "int")        return PlyProperty::Type::INT32;
        else if (t == "uint32" || t == "uint")      return PlyProperty::Type::UINT32;
        else if (t == "float32" || t == "float")    return PlyProperty::Type::FLOAT32;
        else if (t == "float64" || t == "double")   return PlyProperty::Type::FLOAT64;
        return PlyProperty::Type::INVALID;
    }

    template<typename T>
    inline uint8_t * resize(void * v, std::size_t newSize) {
        auto vec = static_cast<std::vector<T> *>(v);
        vec->resize(newSize);
        return reinterpret_cast<uint8_t *>(vec->data());
    }

    inline void resize_vector(const PlyProperty::Type t, void * v, std::size_t newSize, uint8_t *& ptr) {
        switch (t) {
        case PlyProperty::Type::INT8:       ptr = resize<std::int8_t>(v, newSize);   break;
        case PlyProperty::Type::UINT8:      ptr = resize<std::uint8_t>(v, newSize);  break;
        case PlyProperty::Type::INT16:      ptr = resize<std::int16_t>(v, newSize);  break;
        case PlyProperty::Type::UINT16:     ptr = resize<std::uint16_t>(v, newSize); break;
        case PlyProperty::Type::INT32:      ptr = resize<std::int32_t>(v, newSize);  break;
        case PlyProperty::Type::UINT32:     ptr = resize<std::uint32_t>(v, newSize); break;
        case PlyProperty::Type::FLOAT32:    ptr = resize<float>(v, newSize);    break;
        case PlyProperty::Type::FLOAT64:    ptr = resize<double>(v, newSize);   break;
        case PlyProperty::Type::INVALID:    throw std::invalid_argument("invalid ply property");
        }
    }

    template <typename T>
    inline PlyProperty::Type property_type_for_type(QVector<T> &) {
        if (std::is_same<T, std::int8_t>::value)          return PlyProperty::Type::INT8;
        else if (std::is_same<T, std::uint8_t>::value)    return PlyProperty::Type::UINT8;
        else if (std::is_same<T, std::int16_t>::value)    return PlyProperty::Type::INT16;
        else if (std::is_same<T, std::uint16_t>::value)   return PlyProperty::Type::UINT16;
        else if (std::is_same<T, std::int32_t>::value)    return PlyProperty::Type::INT32;
        else if (std::is_same<T, std::uint32_t>::value)   return PlyProperty::Type::UINT32;
        else if (std::is_same<T, float>::value)           return PlyProperty::Type::FLOAT32;
        else if (std::is_same<T, double>::value)          return PlyProperty::Type::FLOAT64;
        else return PlyProperty::Type::INVALID;
    }

    class PlyElement {
        void parse_internal(QTextStream & is);
    public:
        PlyElement(QTextStream & istream);
        PlyElement(const QString & name, std::size_t count) : name(name), size(count) {}
        QString name;
        std::size_t size;
        std::vector<PlyProperty> properties;
    };

    inline boost::optional<std::size_t> find_element(const QString key, std::vector<PlyElement> & list) {
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (list[i].name == key) {
                return i;
            }
        }
        return boost::none;
    }

    class PlyFile {

    public:

        PlyFile() {}
        PlyFile(QIODevice &is);

        void read(QIODevice & device);
        bool write(QIODevice & device, const bool asBinary);

        std::vector<PlyElement> & get_elements() { return elements; }

        std::vector<QString> comments;
        std::vector<QString> objInfo;

        template<typename T>
        std::size_t request_properties_from_element(const QString &elementKey, std::vector<QString> propertyKeys, QVector<T> & source, int & missingKeys, const unsigned int listCount = 1) {
            if (get_elements().size() == 0) {
                return 0;
            }

            if (find_element(elementKey, get_elements())) {
                if (std::find(requestedElements.begin(), requestedElements.end(), elementKey) == requestedElements.end()) {
                    requestedElements.push_back(elementKey);
                }
            }
            else {
                return 0;
            }

            // count and verify large enough
            auto instance_counter = [&](const QString & elementKey, const QString & propertyKey) {
                for (const auto & e : get_elements()) {
                    if (e.name != elementKey) {
                        continue;
                    }
                    for (const auto & p : e.properties) {
                        if (p.name == propertyKey) {
                            if (PropertyTable[property_type_for_type(source)].stride != PropertyTable[p.propertyType].stride) {
                                throw std::runtime_error("destination vector is wrongly typed to hold this property");
                            }
                            return e.size;

                        }
                    }
                }
                return std::size_t(0);
            };

            // Check if requested key is in the parsed header
            std::vector<QString> unusedKeys;
            for (const auto & key : propertyKeys) {
                for (const auto & e : get_elements()) {
                    if (e.name != elementKey) {
                        continue;
                    }
                    std::vector<QString> headerKeys;
                    for (const auto & p : e.properties) {
                        headerKeys.push_back(p.name);
                    }

                    if (std::find(headerKeys.begin(), headerKeys.end(), key) == headerKeys.end()) {
                        missingKeys++;
                        unusedKeys.push_back(key);
                    }
                }
            }

            // Not using them? Don't let them affect the propertyKeys count used for calculating array sizes
            for (auto k : unusedKeys) {
                propertyKeys.erase(std::remove(propertyKeys.begin(), propertyKeys.end(), k), propertyKeys.end());
            }
            if (!propertyKeys.size()) {
                return 0;
            }

            // All requested properties in the userDataTable share the same cursor (thrown into the same flat array)
            auto cursor = std::make_shared<DataCursor>();

            std::vector<size_t> instanceCounts;

            for (const auto & key : propertyKeys) {
                if (const auto instanceCount = instance_counter(elementKey, key)) {
                    instanceCounts.push_back(instanceCount);
                    auto result = userDataTable.insert(std::pair<QString, std::shared_ptr<DataCursor>>(make_key(elementKey, key), cursor));
                    if (result.second == false) {
                        throw std::invalid_argument("property has already been requested: " + key.toStdString());
                    }
                }
                else {
                    continue;
                }
            }

            std::size_t totalInstanceSize = [&]() {
                std::size_t t = 0;
                for (const auto c : instanceCounts) {
                    t += c;
                }
                return t;
            }() * listCount;
            source.resize(totalInstanceSize); // this satisfies regular properties; `cursor->realloc` is for list types since tinyply uses single-pass parsing
            cursor->offset = 0;
            cursor->vector = &source;
            cursor->data = reinterpret_cast<std::uint8_t *>(source.data());

            if (listCount > 1) {
                cursor->realloc = true;
                return (totalInstanceSize / propertyKeys.size()) / listCount;
            }

            return totalInstanceSize / propertyKeys.size();
        }

        template<typename T>
        void add_properties_to_element(const QString & elementKey, const QVector<QString> & propertyKeys, QVector<T> & source, const unsigned int listCount = 1, const PlyProperty::Type listType = PlyProperty::Type::INVALID) {
            auto cursor = std::make_shared<DataCursor>();
            cursor->offset = 0;
            cursor->vector = &source;
            cursor->data = reinterpret_cast<uint8_t *>(source.data());

            auto create_property_on_element = [&](PlyElement & e) {
                for (auto key : propertyKeys) {
                    PlyProperty::Type t = property_type_for_type(source);
                    PlyProperty newProp = (listType == PlyProperty::Type::INVALID) ? PlyProperty(t, key) : PlyProperty(listType, t, key, listCount);
                    userDataTable.insert(std::pair<QString, std::shared_ptr<DataCursor>>(make_key(e.name, key), cursor));
                    e.properties.push_back(newProp);
                }
            };

            const auto idx = find_element(elementKey, elements);
            if (idx) {
                PlyElement & e = elements[idx.get()];
                create_property_on_element(e);
            }
            else {
                PlyElement newElement = (listCount == 1) ? PlyElement(elementKey, source.size() / propertyKeys.size()) : PlyElement(elementKey, source.size() / listCount);
                create_property_on_element(newElement);
                elements.push_back(newElement);
            }
        }

        void write_ascii(QTextStream & os);
        void write_binary(QDataStream & os);
    private:

        std::size_t skip_property(const PlyProperty & property, QDataStream &is);
        void skip_property(const PlyProperty & property, QTextStream & is);
        void read_property(PlyProperty::Type t, void * dest, std::size_t & destOffset, QDataStream &is);
        void read_property(PlyProperty::Type t, void * dest, std::size_t & destOffset, QTextStream & is);
        void write_property_ascii(PlyProperty::Type t, QTextStream & os, std::uint8_t * src, std::size_t & srcOffset);
        void write_property_binary(PlyProperty::Type t, QDataStream & os, std::uint8_t * src, std::size_t & srcOffset);

        bool parse_header(QIODevice & device);
        void write_header(QIODevice & device);

        void read_header_format(QTextStream &is);
        void read_header_element(QTextStream &is);
        void read_header_property(QTextStream &is);
        void read_header_text(QString line, std::vector<QString> &place, const int erase = 0);

        template<typename T>
        void read_internal(T & is);

        bool isBinary{false};
        bool isBigEndian{false};

        std::map<QString, std::shared_ptr<DataCursor>> userDataTable;

        std::vector<PlyElement> elements;
        std::vector<QString> requestedElements;
    };

} // namesapce tinyply
