#include <string>
#include <vector>

#include <toml11/parser.hpp>

toml::value toml_parse(const std::vector<unsigned char> & blob, const std::string & filename) {
    return toml::parse(blob, filename);
}
