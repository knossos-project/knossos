#include "NeuroLucida.h"

#include <QDebug>

//#define BOOST_SPIRIT_DEBUG_OUT
//#define BOOST_SPIRIT_DEBUG
//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

#include <iostream>

namespace asc_parser {
    using namespace boost::spirit::x3;

    auto comment = [](auto & ctx){
        std::cout << _attr(ctx) << std::endl;
    };
    auto node = [](auto & ctx){
        std::cout << _attr(ctx) << std::endl;
    };
    auto nodes = [](auto & ctx){
        for (const auto & elem : _attr(ctx)) {
            std::cout << elem << std::endl;
        }
    };

    auto foo = [](auto & ctx){
        std::cout << _attr(ctx) << std::endl;
    };

    const rule<class branch_id> branch_ = "branch";
    const rule<class split_id> split_ = "split";
    const rule<class tree_id> tree_ = "tree";
    const rule<class asc_id> asc_ = "asc";

    const auto comment_ = lexeme['"' >> *(char_ - '"') >> '"'][comment];
    const auto color_ = lit("(Color ") >> ((lit("RGB (") >> int_ >> ',' >> int_ >> ',' >> int_ >> ')') | "Blue" | "Cyan" | "DarkCyan" | "DarkGreen" | "DarkRed" | "DarkYellow" | "Green" | "Magenta" | "MoneyGreen" | "SkyBlue" | "Yellow") >> ')';// TODO colors
    const auto type_ = '(' >> (lit("Apical") | "Dendrite") >> ')';
    const auto node_ = ('(' >> double_[foo] >> double_ >> double_ >> double_ >> ')' >> ';' >> (lit("Root") | (int_ >> -(',' >> (int_ | ('R' >> *('-' >> int_)))))));// TODO after id
    const auto branch__def = *node_ >> (split_ | "Normal");
    const auto split__def = '(' >> branch_ >> -('|' >> branch_) >> -('|' >> branch_) >> ')' >> ';' >> "End of split";
    const auto tree__def = '(' >> -comment_ >> color_ >> type_ >> branch_ >> ')' >> ';' >> "End of tree";
    const auto asc__def = lit(";	V3 text file written for MicroBrightField products.") >> "(ImageCoords)"
                     >> '(' >> -comment_ >> color_ >> "(CellBody)" >> *node_ >> ')' >> ';' >> "End of contour"
                     >> *tree_;
    BOOST_SPIRIT_DEFINE(branch_)
    BOOST_SPIRIT_DEFINE(split_)
    BOOST_SPIRIT_DEFINE(tree_)
    BOOST_SPIRIT_DEFINE(asc_);
}

void parseNeuroLucida(QIODevice && file) {
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("*.asc open failed");
    }
    auto stream = file.readAll();
    auto first = std::begin(stream);
    bool res = boost::spirit::x3::phrase_parse(first, std::end(stream), asc_parser::asc_, boost::spirit::x3::space);
    if (first != std::end(stream)) {
        qDebug() << "didn’t parse everything";
    }
    qDebug() << res;
}
