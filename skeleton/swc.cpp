#include "swc.h"

#include "dataset.h"
#include "skeletonizer.h"

#include <QDebug>

#include <boost/optional.hpp>
#include <boost/spirit/include/karma_format.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/home/karma.hpp>
#include <boost/spirit/home/x3.hpp>

void parseSWC(QIODevice && file) {
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("*.swc open failed");
    }
    const auto stream = file.readAll();
    auto first = std::cbegin(stream);

    auto treeID = Skeletonizer::singleton().addTree().treeID;
    const auto node = [treeID](auto & ctx){
        using namespace boost::fusion;
        const auto & attr = _attr(ctx);
        const auto pos = floatCoordinate(at_c<2>(attr), at_c<3>(attr), at_c<4>(attr)) * 1000.0 / Dataset::current().scale;
        const auto radius = at_c<5>(attr) * 1000.0 / Dataset::current().scale.x;
        auto node = Skeletonizer::singleton().addNode(at_c<0>(attr), radius, treeID, pos, VIEWPORT_UNDEFINED, 1, boost::none, false);
        if (const auto connectedNode = Skeletonizer::singleton().findNodeByNodeID(at_c<6>(attr))) {
            Skeletonizer::singleton().addSegment(*connectedNode, node.get());
        }
    };
    using namespace boost::spirit::x3;
    const auto comment_ = lexeme['#' >> *(char_ - eol) >> (eol|eoi)];
    const auto node_ = (int_ >> int_ >> double_ >> double_ >> double_ >> double_ >> int_)[node];
    const bool res = boost::spirit::x3::phrase_parse(first, std::end(stream), *(node_ | comment_), boost::spirit::x3::space);

    if (!res || first != std::end(stream)) {
        Skeletonizer::singleton().clearSkeleton();
        throw std::runtime_error("parseSWC parsing failed");
    }
}

using namespace boost::spirit;
static floatCoordinate multiplier;

std::ostream & operator<< (std::ostream & os, const nodeListElement & node) {
    boost::optional<decltype(nodeListElement::nodeID)> parent_id;
    for (const auto & segment : node.segments) {
        if (segment.source.nodeID != node.nodeID) {
            parent_id = segment.source.nodeID;
            break;
        }
    }
    const auto pos = static_cast<floatCoordinate>(node.position) * multiplier;
    std::string nodeLine;
    os << karma::format_delimited(uint_ << 0 << double_ << double_ << double_ << double_,
                                  boost::spirit::ascii::space,
                                  node.nodeID, pos.x(), pos.y(), pos.z(), node.radius * multiplier.x);
    // instead of a second format call here, the above rule could be:
    // uint_ << 0 << double_ << double_ << double_ << double_ << &bool_(true) << uint_ | uint_ << 0 << double_ << double_ << double_ << double_ << -1
    // but that is redundant and produces a trailing space.
    os << karma::format(&bool_(true) << uint_ | -1, !!parent_id, parent_id.value_or(0));
    return os;
}

template <typename OutputIterator>
struct nodeList : karma::grammar<OutputIterator, decltype(treeListElement::nodes)()> {
    nodeList() : nodeList::base_type(list) {
        list =  node << *('\n' << node) << '\n';
        node = karma::stream;
    }
    karma::rule<OutputIterator, decltype(treeListElement::nodes)()> list;
    karma::rule<OutputIterator, nodeListElement()> node;
};

void writeSWC(QIODevice & file, const treeListElement & tree, const bool pixelSpace) {
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("*.swc open failed");
    }
    std::string swcText;
    std::back_insert_iterator<std::string> sink{swcText};
    multiplier = pixelSpace ? floatCoordinate(1, 1, 1) : Dataset::current().scale / 1000.f;
    karma::generate(sink, nodeList<std::back_insert_iterator<std::string>>{}, tree.nodes);
    file.write(swcText.data());
}
