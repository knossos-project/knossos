// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy,
// distribute, and modify this file as you see fit.
// Authored in 2015 by Dimitri Diakopoulos (http://www.dimitridiakopoulos.com)
// https://github.com/ddiakopoulos/tinyply

#include "tinyply.h"

using namespace tinyply;

//////////////////
// PLY Property //
//////////////////

PlyProperty::PlyProperty(QTextStream & is) : isList(false) {
    parse_internal(is);
}

void PlyProperty::parse_internal(QTextStream & is) {
    QString type;
    is >> type;
    if (type == "list") {
        QString countType;
        is >> countType >> type;
        listType = property_type_from_string(countType);
        isList = true;
    }
    propertyType = property_type_from_string(type);
    is >> name;
}

/////////////////
// PLY Element //
/////////////////

PlyElement::PlyElement(QTextStream & is) {
    parse_internal(is);
}

void PlyElement::parse_internal(QTextStream & is) {
    is >> name >> size;
}

//////////////
// PLY File //
//////////////

PlyFile::PlyFile(QIODevice & device) {
    if (!parse_header(device)) {
        throw std::runtime_error("file is not ply or encounted junk in header");
    }
}

bool PlyFile::parse_header(QIODevice & device) {
    QString line;
    while ((line = device.readLine()) > 0) {
        QTextStream ls(&line);
        QString token;
        ls >> token;
        if (token == "ply" || token == "PLY" || token == "") {
            continue;
        }
        else if (token == "comment")    read_header_text(line, comments, 8);
        else if (token == "format")     read_header_format(ls);
        else if (token == "element")    read_header_element(ls);
        else if (token == "property")   read_header_property(ls);
        else if (token == "obj_info")   read_header_text(line, objInfo, 9);
        else if (token == "end_header") break;
        else return false;
    }
    return true;
}

void PlyFile::read_header_text(QString line, std::vector<QString>& place, const int erase) {
    place.push_back((erase > 0) ? line.remove(0, erase) : line);
}

void PlyFile::read_header_format(QTextStream & is) {
    QString s;
    is >> s;
    isBigEndian = (s == "binary_big_endian");
    isBinary = (s == "binary_little_endian") || isBigEndian;
}

void PlyFile::read_header_element(QTextStream & is) {
    get_elements().emplace_back(is);
}

void PlyFile::read_header_property(QTextStream & is) {
    get_elements().back().properties.emplace_back(is);
}

std::size_t PlyFile::skip_property(const PlyProperty & property, QDataStream & is) {
    static std::vector<char> skip(PropertyTable[property.propertyType].stride);
    if (property.isList) {
        std::size_t listSize = 0;
        std::size_t dummyCount = 0;
        read_property(property.listType, &listSize, dummyCount, is);
        for (std::size_t i = 0; i < listSize; ++i) {
            is.readRawData(skip.data(), PropertyTable[property.propertyType].stride);
        }
        return listSize;
    } else {
        is.readRawData(skip.data(), PropertyTable[property.propertyType].stride);
        return 0;
    }
}

void PlyFile::skip_property(const PlyProperty & property, QTextStream & is) {
    QString skip;
    if (property.isList) {
        int listSize;
        is >> listSize;
        for (int i = 0; i < listSize; ++i) {
            is >> skip;
        }
    } else {
        is >> skip;
    }
}

void PlyFile::read_property(PlyProperty::Type t, void * dest, std::size_t & destOffset, QDataStream & is) {
    static std::vector<char> src(PropertyTable[t].stride);
    is.readRawData(src.data(), PropertyTable[t].stride);
    if (is.status() != QDataStream::Ok) {
        throw std::invalid_argument("Failed to read property because of malformed ply file: stream status " + std::to_string(is.status()));
    }

    switch (t) {
    case PlyProperty::Type::INT8:       ply_cast<std::int8_t>(dest, src.data());   break;
    case PlyProperty::Type::UINT8:      ply_cast<std::uint8_t>(dest, src.data());  break;
    case PlyProperty::Type::INT16:      ply_cast<std::int16_t>(dest, src.data());  break;
    case PlyProperty::Type::UINT16:     ply_cast<std::uint16_t>(dest, src.data()); break;
    case PlyProperty::Type::INT32:      ply_cast<std::int32_t>(dest, src.data());  break;
    case PlyProperty::Type::UINT32:     ply_cast<std::uint32_t>(dest, src.data()); break;
    case PlyProperty::Type::FLOAT32:    ply_cast_float<float>(dest, src.data());   break;
    case PlyProperty::Type::FLOAT64:    ply_cast_double<double>(dest, src.data()); break;
    case PlyProperty::Type::INVALID:    throw std::invalid_argument("invalid ply property");
    }
    destOffset += PropertyTable[t].stride;
}

void PlyFile::read_property(PlyProperty::Type t, void * dest, std::size_t & destOffset, QTextStream & is) {
    switch (t) {
    case PlyProperty::Type::INT8:       *((std::int8_t *)dest) = ply_read_ascii<std::int32_t>(is);   break;
    case PlyProperty::Type::UINT8:      *((std::uint8_t *)dest) = ply_read_ascii<std::uint32_t>(is); break;
    case PlyProperty::Type::INT16:      ply_cast_ascii<std::int16_t>(dest, is);                      break;
    case PlyProperty::Type::UINT16:     ply_cast_ascii<std::uint16_t>(dest, is);                     break;
    case PlyProperty::Type::INT32:      ply_cast_ascii<std::int32_t>(dest, is);                      break;
    case PlyProperty::Type::UINT32:     ply_cast_ascii<std::uint32_t>(dest, is);                     break;
    case PlyProperty::Type::FLOAT32:    ply_cast_ascii<float>(dest, is);                             break;
    case PlyProperty::Type::FLOAT64:    ply_cast_ascii<double>(dest, is);                            break;
    case PlyProperty::Type::INVALID:    throw std::invalid_argument("invalid ply property");
    }
    destOffset += PropertyTable[t].stride;
}

void PlyFile::write_property_ascii(PlyProperty::Type t, QTextStream & os, uint8_t * src, std::size_t & srcOffset) {
    switch (t) {
    case PlyProperty::Type::INT8:       os << static_cast<std::int32_t>(*reinterpret_cast<std::int8_t*>(src));    break;
    case PlyProperty::Type::UINT8:      os << static_cast<std::uint32_t>(*reinterpret_cast<std::uint8_t*>(src));  break;
    case PlyProperty::Type::INT16:      os << *reinterpret_cast<std::int16_t*>(src);     break;
    case PlyProperty::Type::UINT16:     os << *reinterpret_cast<std::uint16_t*>(src);    break;
    case PlyProperty::Type::INT32:      os << *reinterpret_cast<std::int32_t*>(src);     break;
    case PlyProperty::Type::UINT32:     os << *reinterpret_cast<std::uint32_t*>(src);    break;
    case PlyProperty::Type::FLOAT32:    os << *reinterpret_cast<float*>(src);       break;
    case PlyProperty::Type::FLOAT64:    os << *reinterpret_cast<double*>(src);      break;
    case PlyProperty::Type::INVALID:    throw std::invalid_argument("invalid ply property");
    }
    os << " ";
    srcOffset += PropertyTable[t].stride;
}

void PlyFile::write_property_binary(PlyProperty::Type t, QDataStream & os, std::uint8_t * src, std::size_t & srcOffset) {
    os.writeRawData((char *)src, PropertyTable[t].stride);
    srcOffset += PropertyTable[t].stride;
}

void PlyFile::read(QIODevice & device) {
    if (isBinary) {
        QDataStream is(&device);
        is.setByteOrder(isBigEndian? QDataStream::BigEndian : QDataStream::LittleEndian);
        read_internal(is);
    } else {
        QTextStream is(&device);
        read_internal(is);
    }
}

bool PlyFile::write(QIODevice & device, const bool asBinary) {
    isBinary = asBinary;
    write_header(device);
    if (isBinary) {
        QDataStream ds(&device);
        write_binary(ds);
        return ds.status() == QDataStream::Ok;
    } else {
        QTextStream ts(&device);
        write_ascii(ts);
        return ts.status() == QTextStream::Ok;
    }
}

void PlyFile::write_binary(QDataStream & os) {
    for (const auto & e : elements) {
        for (std::size_t i = 0; i < e.size; ++i) {
            for (const auto & p : e.properties) {
                auto & cursor = userDataTable[make_key(e.name, p.name)];
                if (p.isList) {
                    std::uint8_t listSize[4] = {0, 0, 0, 0};
                    memcpy(listSize, &p.listCount, sizeof(std::uint32_t));
                    std::size_t dummyCount = 0;
                    write_property_binary(p.listType, os, listSize, dummyCount);
                    for (unsigned int j = 0; j < p.listCount; ++j) {
                        write_property_binary(p.propertyType, os, (cursor->data + cursor->offset), cursor->offset);
                    }
                } else {
                    write_property_binary(p.propertyType, os, (cursor->data + cursor->offset), cursor->offset);
                }
            }
        }
    }
}

void PlyFile::write_ascii(QTextStream & os) {
    for (const auto & e : elements) {
        for (size_t i = 0; i < e.size; ++i) {
            for (const auto & p : e.properties) {
                auto & cursor = userDataTable[make_key(e.name, p.name)];
                if (p.isList) {
                    os << p.listCount << " ";
                    for (unsigned int j = 0; j < p.listCount; ++j) {
                        write_property_ascii(p.propertyType, os, (cursor->data + cursor->offset), cursor->offset);
                    }
                } else {
                    write_property_ascii(p.propertyType, os, (cursor->data + cursor->offset), cursor->offset);
                }
            }
            os << "\n";
        }
    }
}

void PlyFile::write_header(QIODevice & device) {
    QTextStream tmpStream(&device);
    tmpStream.setLocale(QLocale::C);
    
    tmpStream << "ply\n";
    if (isBinary) {
        tmpStream << ((isBigEndian) ? "format binary_big_endian 1.0\n" : "format binary_little_endian 1.0\n");
    } else {
        tmpStream << "format ascii 1.0\n";
    }
    for (const auto & comment : comments) {
        tmpStream << "comment " << comment << "\n";
    }
    
    for (auto & e : elements) {
        tmpStream << "element " << e.name << " " << e.size << "\n";
        for (const auto & p : e.properties) {
            if (p.isList) {
                tmpStream << "property list " << PropertyTable[p.listType].str << " "
                << PropertyTable[p.propertyType].str << " " << p.name << "\n";
            } else {
                tmpStream << "property " << PropertyTable[p.propertyType].str << " " << p.name << "\n";
            }
        }
    }
    tmpStream << "end_header" << "\n";
}

template<typename T>
void PlyFile::read_internal(T & is) {
    for (auto & element : get_elements()) {
        if (std::find(requestedElements.begin(), requestedElements.end(), element.name) != requestedElements.end()) {
            for (std::size_t count = 0; count < element.size; ++count) {
                for (auto & property : element.properties) {
                    if (auto & cursor = userDataTable[make_key(element.name, property.name)]) {
                        if (property.isList) {
                            std::size_t listSize = 0;
                            std::size_t dummyCount = 0;
                            read_property(property.listType, &listSize, dummyCount, is);
                            for (std::size_t i = 0; i < listSize; ++i) {
                                read_property(property.propertyType, (cursor->data + cursor->offset), cursor->offset, is);
                            }
                        } else {
                            read_property(property.propertyType, (cursor->data + cursor->offset), cursor->offset, is);
                        }
                    } else {
                        skip_property(property, is);
                    }
                }
            }
        }
    };
}
template void PlyFile::read_internal(QDataStream & is);
template void PlyFile::read_internal(QTextStream & is);
