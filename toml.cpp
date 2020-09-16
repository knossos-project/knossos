#include <toml/parser.hpp>

toml::value toml_parse(const std::string & filename) {
    return toml::parse(filename);
}
