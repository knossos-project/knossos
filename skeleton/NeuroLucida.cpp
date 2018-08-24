#include "NeuroLucida.h"

#include "dataset.h"
#include "skeletonizer.h"

#include <QColor>
#include <QDebug>

#include <boost/foreach.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/optional/optional_io.hpp>
//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>

#include <iostream>
#include <functional>
#include <string>
#include <vector>

//namespace asc_parser {
//struct hardCodedColor {
//    std::string name;
//};
//}
//BOOST_FUSION_ADAPT_STRUCT(asc_parser::hardCodedColor, name)

namespace asc_parser {
using namespace boost::spirit::x3;

const auto comment = [](auto & ctx){
    std::cout << "comment: " << _attr(ctx) << std::endl;
};
boost::optional<QColor> lastTreeColor;
const auto treeColor = [](auto & ctx){
    using namespace boost::fusion;
    lastTreeColor = QColor::fromRgb(at_c<0>(_attr(ctx)), at_c<1>(_attr(ctx)), at_c<2>(_attr(ctx)));
};
const auto hardCodedColor = [](auto &){
    lastTreeColor = boost::none;// K chooses a color itself
};
decltype(treeListElement::treeID) currentTreeId;
std::vector<std::reference_wrapper<nodeListElement>> branchStack;
boost::optional<nodeListElement&> lastNode;
const auto tree = [](auto & ctx){
    using namespace boost::fusion;
    auto & tree = Skeletonizer::singleton().addTree();
    Skeletonizer::singleton().setComment(tree, QString::fromStdString(_attr(ctx)));
    if (lastTreeColor) {
        tree.color = lastTreeColor.get();
        lastTreeColor = boost::none;
    }
    currentTreeId = tree.treeID;
    lastNode = boost::none;
};
const auto node = [](auto & ctx){
    using namespace boost::fusion;
    const auto & attr = _attr(ctx);
    const auto pos = floatCoordinate(0, Dataset::current().boundary.y * 0.5, Dataset::current().boundary.z) + floatCoordinate(-at_c<1>(attr), at_c<0>(attr), at_c<2>(attr)) * 1000.0 / Dataset::current().scale;
    auto node = Skeletonizer::singleton().addNode(boost::none, 0.5 * at_c<3>(attr), currentTreeId, pos, VIEWPORT_UNDEFINED, 1, boost::none, false);
    if (node) {
        if (lastNode) {
            Skeletonizer::singleton().addSegment(lastNode.get(), node.get());
        }
        lastNode = node;
    }
};
const auto branchBegin = [](auto&){
    assert(lastNode);
    branchStack.emplace_back(std::ref(lastNode.get()));
};

const auto branchEnd = [](auto&){
    if (!branchStack.empty()) {
        lastNode = branchStack.back().get();
        branchStack.pop_back();
    } else {
        lastNode = boost::none;
    }
};

const rule<class branch_id> branch_ = "branch";
const rule<class split_id> split_ = "split";
const rule<class tree_id> tree_ = "tree";
const rule<class asc_id> asc_ = "asc";

const auto comment_ = '"' >> lexeme[*(char_ - '"')][comment] >> '"';
const auto treeColor_ = lit("(Color ") >> lit("RGB (") >> (int_ >> ',' >> int_ >> ',' >> int_)[treeColor] >> ')' >> ')';
const auto hardCodedColor_ = lit("(Color ") >> ((*(char_ - ')')) | string("Blue") | string("Cyan") | string("DarkCyan") | string("DarkGreen") | string("DarkRed")| string("DarkYellow") | string("Green") | string("Magenta") | string("MoneyGreen") | string("SkyBlue") | string("Yellow")) >> ')';
const auto color_ = treeColor_ | hardCodedColor_[hardCodedColor];
const auto type_ = '(' >> (string("Apical")[tree] | string("CellBody")[tree] | string("Dendrite")[tree]) >> ')';
const auto node_ = ('(' >> (double_ >> double_ >> double_ >> double_)[node] >> ')' >> ';' >> (lit("Root") | (int_ >> -(',' >> (int_ | ('R' >> *('-' >> int_)))))));// TODO after id
const auto branch__def = *node_ >> (split_ | lit("Normal")[branchEnd]);
const auto split__def = lit('(')[branchBegin] >> branch_ >> *('|' >> branch_) >> ')' >> ';' >> "End of split";
const auto tree__def = '(' >> -comment_ >> color_ >> type_ >> branch_ >> ')' >> ';' >> "End of tree";
const auto asc__def = lit(";\tV3 text file written for MicroBrightField products.") >> "(ImageCoords)"
                 >> '(' >> -comment_ >> color_ >> type_ >> *node_ >> ')' >> ';' >> "End of contour"
                 >> *tree_;
BOOST_SPIRIT_DEFINE(branch_, split_, tree_, asc_)
}

void parseNeuroLucida(QIODevice && file) {
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("*.asc open failed");
    }
    auto stream = file.readAll();
    auto first = std::begin(stream);

    const auto blockedSignals = Skeletonizer::singleton().blockSignals(true);
    bool res = boost::spirit::x3::phrase_parse(first, std::end(stream), asc_parser::asc_, boost::spirit::x3::space);
    Skeletonizer::singleton().blockSignals(blockedSignals);
    emit Skeletonizer::singleton().resetData();

    qDebug() << res << (first == std::end(stream));
}
